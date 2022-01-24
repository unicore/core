using namespace eosio;
// namespace eosio {
  /**
   * @brief      Создание вестинг-баланса
   * Внутренний метод, используемый при обратном обмене силы сообщества на котировочный токен. 
   * Обеспечивает линейную выдачу жетонов в продолжительности времени. 
   * @param[in]  owner   The owner
   * @param[in]  amount  The amount
   */
	void make_vesting_action(eosio::name owner, eosio::name host, eosio::name contract, eosio::asset amount, uint64_t vesting_seconds, eosio::name type){
	    eosio::check(amount.is_valid(), "Amount not valid");
	    // eosio::check(amount.symbol == _SYM, "Not valid symbol for this vesting contract");
	    eosio::check(is_account(owner), "Owner account does not exist");
	    
	    vesting_index vests (_me, owner.value);
	    
	    vests.emplace(_me, [&](auto &v){
	      v.id = vests.available_primary_key();
        v.type = type;
        v.host = host;
	      v.owner = owner;
        v.contract = contract;
	      v.amount = amount;
        v.available = asset(0, amount.symbol);
        v.withdrawed = asset(0, amount.symbol);
	      v.startat = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
	      v.duration = vesting_seconds;
	    });

	}

  /**
   * @brief      Метод обновления вестинг-баланса.  
   * Обновляет вестинг-баланс до актуальных параметров продолжительности. 
   * @param[in]  op    The operation
   */
  [[eosio::action]] void unicore::refreshsh (eosio::name owner, uint64_t id){
    require_auth(owner);
    vesting_index vests(_me, owner.value);
    auto v = vests.find(id);
    eosio::check(v != vests.end(), "Vesting object does not exist");
    
    if (eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() ) > v->startat){
      
      auto elapsed_seconds = (eosio::time_point_sec(eosio::current_time_point().sec_since_epoch()) - v->startat).to_seconds();
      eosio::asset available;
    
      if( elapsed_seconds < v->duration){
        available = v->amount * elapsed_seconds / v->duration;
      } else {
        available = v->amount;
      }

      available = available - v->withdrawed;
      vests.modify(v, owner, [&](auto &m){
      	m.available = available;
      });

    }
  }

  /**
   * @brief      Вывод вестинг-баланса
   * Обеспечивает вывод доступных средств из вестинг-баланса. 
   *
   * @param[in]  op    The operation
   */
  [[eosio::action]] void unicore::withdrawsh(eosio::name owner, uint64_t id){
    require_auth(owner);
    vesting_index vests(_me, owner.value);
    auto v = vests.find(id);
    eosio::check(v != vests.end(), "Vesting object does not exist");
    eosio::check((v->available).amount > 0, "Only positive amount can be withdrawed");
    account_index account(_me, (v->host).value);
    auto acc = account.find((v->host).value);

    action(
        permission_level{ _me, "active"_n },
        v->contract, "transfer"_n,
        std::make_tuple( _me, owner, v->available, std::string("Vesting Withdrawed")) 
    ).send();

    vests.modify(v, owner, [&](auto &m){
        m.withdrawed = v->withdrawed + v->available;
        m.available = eosio::asset(0, (v->available).symbol);
      });

    if (v->withdrawed == v->amount){
    	vests.erase(v);
    };
    
  };

  /**
   * @brief      Метод вывода силового финансового потока
   * withdraw power quant (withpowerun)
   * Позволяет вывести часть финансового потока, направленного на держателя силы
  */
  [[eosio::action]] void unicore::withpbenefit(eosio::name username, eosio::name host){
    require_auth(username);
    pstats_index pstats(_me, username.value);
    auto pstat = pstats.find(host.value);
    eosio::check(pstat != pstats.end(), "Power Stat Object is not found. Refresh your balances first.");
    eosio::check((pstat -> pflow_available_in_asset).amount > 0, "Not enough tokens for transfer");
    account_index account(_me, host.value);

    auto acc = account.find(host.value);
    auto root_symbol = acc->get_root_symbol();
    auto on_withdraw = pstat -> pflow_available_in_asset;
    
    if (on_withdraw.amount > 0){

      action(
          permission_level{ _me, "active"_n },
          acc->root_token_contract, "transfer"_n,
          std::make_tuple( _me, username, on_withdraw, std::string("PFLOW-" + (name{username}.to_string() + "-" + (name{host}).to_string()) ))
      ).send();


      pstats.modify(pstat, _me, [&](auto &ps){
        
        ps.total_available_in_asset -= on_withdraw;
        
        ps.pflow_available_segments -= (pstat -> pflow_available_in_asset).amount * TOTAL_SEGMENTS;
        ps.pflow_available_in_asset = asset(0, root_symbol);
        ps.pflow_withdrawed += on_withdraw;
      });    
    }

  }




  // [[eosio::action]] void unicore::refreshru(eosio::name username, eosio::name host){


  // }



  /**
   * @brief      Метод обновления силового финансового потока
   * 
   * Позволяет обновить доступную часть финансового потока, направленного на держателя силы, 
   * а так же собрать доступные реферальные балансы в один объект
  */
  [[eosio::action]] void unicore::refreshpu(eosio::name username, eosio::name host){
    require_auth(username);
    
    account_index account(_me, host.value);

    auto acc = account.find(host.value);
    auto root_symbol = acc->get_root_symbol();
    eosio::check(acc != account.end(), "Host is not found");
    

    pstats_index pstats(_me, username.value);
    auto pstat = pstats.find(host.value);
    auto min_pool_id = 0;

    bool skip = false;

    if (pstat != pstats.end())
      min_pool_id = pstat -> pflow_last_withdrawed_pool_id;

    

   
    sincome_index sincomes(_me, host.value);
    auto sincome = sincomes.lower_bound(min_pool_id);
    
    print("here1");

    if ((min_pool_id != 0) && (sincome != sincomes.end()))
      sincome++;

    uint64_t count2 = 0;
    
    while(sincome != sincomes.end() && count2 < 50) {
    // if (sincome != sincomes.end()){

      plog_index plogs(_me, username.value);

      auto pool_idwithhost_idx = plogs.template get_index<"hostpoolid"_n>();
      auto log_id = combine_ids(host.value, sincome-> pool_id);


      auto plog = pool_idwithhost_idx.find(log_id);  
      
      if (plog == pool_idwithhost_idx.end()){
        if (plog != pool_idwithhost_idx.begin()){
          plog--;

          if ( plog == pool_idwithhost_idx.end())
            skip = true;

        } else {
          skip = true;
        }
      } 

      if (plog -> pool_id < sincome -> pool_id) { //Только те plog принимаем в распределение, которые были преобретены до текущего раунда
        if ( sincome->liquid_power > 0 && sincome-> hfund_in_segments > 0 && !skip ) { 
          
          eosio::check(plog -> power <= sincome->liquid_power, "System error2");
          uint128_t user_segments = (double)plog -> power / (double)sincome -> liquid_power * sincome -> hfund_in_segments;
          
          uint64_t user_segments_64 = (uint64_t)user_segments / TOTAL_SEGMENTS;
          eosio::asset segment_asset = asset(user_segments_64, root_symbol);
          
          print(" usegments: ", (double)user_segments);
          print(" hfund_in_segments: ", (double)sincome->hfund_in_segments);
          print(" is_equal: ", (double)sincome->hfund_in_segments == (double)user_segments);
          print(" distr_segmnts: ", (double)sincome->distributed_segments);

          eosio::check((double)sincome->distributed_segments + (double)user_segments <= (double)sincome->hfund_in_segments, "Overflow fund on distibution event.");

          sincomes.modify(sincome, _me, [&](auto &s){
            s.distributed_segments += user_segments;
          });
          
          if (pstat == pstats.end()){
            print("here1");
            pstats.emplace(_me, [&](auto &ps){
              ps.host = host;
              ps.total_available_in_asset = segment_asset;
          
              ps.pflow_last_withdrawed_pool_id = sincome -> pool_id;
              ps.pflow_available_segments = user_segments;
              ps.pflow_available_in_asset = segment_asset;
              ps.pflow_withdrawed = asset(0, root_symbol);

              ps.ref_available_in_asset = asset(0, root_symbol);
              ps.ref_available_segments = 0;
              ps.ref_withdrawed = asset(0, root_symbol);
            });

            pstat = pstats.find(host.value);

          } else {
            pstats.modify(pstat, _me, [&](auto &ps){
              ps.total_available_in_asset += segment_asset;
              ps.pflow_last_withdrawed_pool_id = sincome -> pool_id;
              ps.pflow_available_segments += user_segments;
              ps.pflow_available_in_asset += segment_asset;
            });    

          }
          dlog_index dlogs(_me, username.value);
          print("here2");
          dlogs.emplace(_me, [&](auto &dl){
            dl.id = dlogs.available_primary_key();
            dl.host = host;
            dl.pool_id = sincome -> pool_id;
            dl.segments = user_segments;
            dl.amount = segment_asset;
          });


          //TODO Удалить все устаревшие plog
          //Устаревшие это те, которые больше никогда не будут учитываться в распределении.
          //(есть более новые актуальные записи)
          
        } else { 
              print(" on skip: ", skip);
              if (pstat == pstats.end()){
                pstats.emplace(_me, [&](auto &ps){
                  ps.host = host;
                  ps.total_available_in_asset = asset(0, root_symbol);
                  ps.pflow_last_withdrawed_pool_id = sincome -> pool_id;
                  ps.pflow_available_segments = 0;
                  ps.pflow_available_in_asset = asset(0, root_symbol);
                  ps.pflow_withdrawed = asset(0, root_symbol);

                  ps.ref_available_in_asset = asset(0, root_symbol);
                  ps.ref_available_segments = 0;
                  ps.ref_withdrawed = asset(0, root_symbol);

                });

                pstat = pstats.begin();
              } else {
                pstats.modify(pstat, _me, [&](auto &ps){
                  ps.pflow_last_withdrawed_pool_id = sincome -> pool_id;
                });          
              }
            
          //TODO чистим plog слева
        }
      }
      sincome++;
      count2++;
    }  


    //Расчет выплат произвоить только для объектов sincome с айди, меньше чем текущий в объекте хоста
    //by host and cycle
    //Проверка лога. Получаем объект измененной силы на пуле. 
    //Его объекта нет, то получаем первый и последний объект на текущем цикле.
    //Если объектов нет, используем силу.  
    //Если в логе под текущим номером пула ничего нет, то провяем последний вывод
    //записи из лога все, кроме первой, если она в текущем цикле.
    //Проверяем выводились ли токены прежде
    //Если нет, расчитываем объем токенов из фонда. 
    //Где храним объект фонда распределения? На хосте. 
    //записываем последний вывод и сколько всего получено
    //считать долю в сегментах
    //выдаю долю в сегментах
    //проверка на новом цикле
  }


  /**
   * @brief      Внутренний метод логирования событий покупки силы
   * Используется для расчета доли владения в финансовом потоке cfund в рамках границы пула
   *
  */
   void log_event_with_shares (eosio::name username, eosio::name host, int64_t new_power){
      account_index accounts(_me, host.value);
      auto acc = accounts.find(host.value);

      plog_index plogs(_me, username.value);
 
      auto pool_idwithhost_idx = plogs.template get_index<"hostpoolid"_n>();
      auto log_ids = combine_ids(host.value, acc->current_pool_id);
      
      auto log = pool_idwithhost_idx.find(log_ids);


      if (log == pool_idwithhost_idx.end()){
        plogs.emplace(_me, [&](auto &l){
          l.id = plogs.available_primary_key();
          l.host = host;
          l.pool_id = acc->current_pool_id;
          l.power = new_power;
          l.cycle_num = acc->current_cycle_num;
          l.pool_num = acc->current_pool_num;
        });

      } else {
        pool_idwithhost_idx.modify(log, _me, [&](auto &l){
          l.power = new_power;
        });
      }

      sincome_index sincome(_me, host.value);
      auto sinc = sincome.find(acc->current_pool_id);
      
      rate_index rates(_me, host.value);
      auto rate = rates.find(acc->current_pool_num - 1);
      eosio::name main_host = acc->get_ahost();
      auto root_symbol = acc->get_root_symbol();

      market_index market(_me, host.value);
      auto itr = market.find(0);
      auto liquid_power = acc->total_shares - itr->base.balance.amount;

      if (sinc == sincome.end()){
        sincome.emplace(_me, [&](auto &s){
            s.max = rate -> system_income;
            s.pool_id = acc->current_pool_id;
            s.ahost = main_host;
            s.cycle_num = acc->current_cycle_num;
            s.pool_num = acc->current_pool_num;
            s.liquid_power = liquid_power;
            s.total = asset(0, root_symbol);
            s.paid_to_sys = asset(0, root_symbol);
            s.paid_to_dacs = asset(0, root_symbol);
            s.paid_to_cfund = asset(0, root_symbol);
            s.paid_to_hfund = asset(0, root_symbol);
            s.paid_to_refs = asset(0, root_symbol);
            s.hfund_in_segments = 0;
            s.distributed_segments = 0;
        }); 

      } else {

        sincome.modify(sinc, _me, [&](auto &s){
          s.liquid_power = liquid_power;
        });
      } 
      // auto idx = votes.template get_index<"host"_n>();
      // auto i_bv = idx.lower_bound(host.value);


   } 


  /**
   * @brief      Метод покупки силы сообщества
   * Обеспечивает покупку силы сообщества за котировочный токен по внутренней рыночной цене, определяемой с помощью алгоритма банкор. 
   * @param[in]  buyer   The buyer
   * @param[in]  host    The host
   * @param[in]  amount  The amount
   */
	uint64_t unicore::buyshares_action ( eosio::name buyer, eosio::name host, eosio::asset amount, eosio::name code, bool is_frozen ){
		account_index accounts(_me, host.value);
		partners2_index users(_partners,_partners.value);
    auto user = users.find(buyer.value);
    print("buyer is: ", user->username);
    // eosio::check(user != users.end(), "User is not registered");


		auto exist = accounts.find(host.value);
		eosio::check(exist != accounts.end(), "Host is not founded");
		eosio::check(exist -> quote_token_contract == code, "Wrong quote token contract");
    eosio::check(exist -> quote_amount.symbol == amount.symbol, "Wrong quote token symbol");
		eosio::check(exist -> power_market_id != ""_n, "Can buy shares only when power market is enabled");

    market_index market(_me, host.value);
		auto itr = market.find(0);
		auto tmp = *itr;
		uint64_t shares_out;
		market.modify( itr, _me, [&]( auto &es ) {
	    	shares_out = (es.direct_convert( amount, eosio::symbol(eosio::symbol_code("POWER"), 0))).amount;
	    });

    eosio::check( shares_out > 0, "Amount is not enought for buy 1 share" );

    power3_index power(_me, host.value);

    auto pexist = power.find(buyer.value);
    

    if (pexist == power.end()){

      power.emplace(_me, [&](auto &p){
      	p.username = buyer;
      	p.power = shares_out;
      	if (is_frozen == false){
          p.staked = shares_out;	
        } else {
          p.frozen = shares_out;
        }
      });

      log_event_with_shares(buyer, host, shares_out);
		
    } else {
			unicore::propagate_votes_changes(host, buyer, pexist->power, pexist->power + shares_out);
		  
      log_event_with_shares(buyer, host, pexist->power + shares_out);

			power.modify(pexist, _me, [&](auto &p){
				p.power += shares_out;
        if (is_frozen == false){
				  p.staked += shares_out;
        } else {
          p.frozen += shares_out;
        }
			});
		};

    // unicore::checkminpwr(host, buyer);
		
    return shares_out;
	};



  /**
   * @brief      Внутренний метод возврата силы при возврате значка.
   * Используется при возврате знака отличия хосту
   *
   * @param[in]  op    The operation
   */

  void unicore::back_shares_with_badge_action (eosio::name host, eosio::name from, uint64_t shares){
    account_index accounts(_me, host.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "Host is not found");
    
    power3_index power_to_idx (_me, host.value);
    auto power_to = power_to_idx.find(from.value);
      
    //modify
    unicore::propagate_votes_changes(host, from, power_to->power, power_to->power - shares);
    log_event_with_shares(from, host, power_to->power - shares);

    power_to_idx.modify(power_to, _me, [&](auto &pt){
      pt.power -= shares;
      pt.with_badges -= shares; 
    });

    market_index market(_me, host.value);
    auto itr = market.find(0);
    
    uint64_t emitbdgspwr = unicore::getcondition(host, "emitbdgspwr");

    if (emitbdgspwr == 0) {
      
      market.modify( itr, _me, [&]( auto& es ) {
        es.base.balance = asset((itr -> base).balance.amount + shares, eosio::symbol(eosio::symbol_code("POWER"), 0));
      });

    } else {

      accounts.modify(acc, _me, [&](auto &a){
        a.total_shares -= shares;
      });  

    }
    

    // unicore::checkminpwr(host, from);

  }


  /**
   * @brief      Внутренний метод эмиссии силы.
   * Используется при выдаче знака отличия  
   *
   * @param[in]  op    The operation
   */

  void unicore::give_shares_with_badge_action (eosio::name host, eosio::name reciever, uint64_t shares){
    account_index accounts(_me, host.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "Host is not found");
    
    power3_index power_to_idx (_me, host.value);
    auto power_to = power_to_idx.find(reciever.value);
    
    // SEPARATE BADGES AND MARKET
    market_index market(_me, host.value);
    
    uint64_t emitbdgspwr = unicore::getcondition(host, "emitbdgspwr");
  
    if (emitbdgspwr == 0){
      auto itr = market.find(0);
  
      eosio::check((itr->base).balance.amount >= shares, "Not enought shares on market");    
      
      market.modify( itr, _me, [&]( auto& es ) {
        es.base.balance = asset((itr -> base).balance.amount - shares, eosio::symbol(eosio::symbol_code("POWER"), 0));
      });

    } 

    accounts.modify(acc, _me, [&](auto &a){
      if (emitbdgspwr > 0){
        a.total_shares += shares;
      }
      a.approved_reports = acc -> approved_reports + 1;
    }); 

    
    


    //Emplace or modify power object of reciever and propagate votes changes;
    if (power_to == power_to_idx.end()) {

      log_event_with_shares(reciever, host, shares);
      power_to_idx.emplace(_me, [&](auto &pt){
        pt.username = reciever;
        pt.power = shares;
        pt.delegated = 0;
        pt.with_badges = shares;
      });
    
    } else {
      
      //modify
      unicore::propagate_votes_changes(host, reciever, power_to->power, power_to->power + shares);
      
      log_event_with_shares(reciever, host, power_to->power + shares);

      power_to_idx.modify(power_to, _me, [&](auto &pt){
        pt.power += shares;
        pt.with_badges += shares; 
      });      
    }   

    // unicore::checkminpwr(host, reciever);
    
  }

  /**
   * @brief      Метод делегирования силы.
   * Позволяет делегировать купленную силу для принятия каких-либо решений в пользу любого аккаунта. 
   *
   * @param[in]  op    The operation
   */

	void unicore::delegate_shares_action(eosio::name from, eosio::name reciever, eosio::name host, uint64_t shares){
	 	require_auth(from);
		power3_index power_from_idx (_me, host.value);
		power3_index power_to_idx (_me, host.value);

		delegation_index delegations(_me, from.value);
		
		account_index accounts(_me, host.value);
		auto acc = accounts.find(host.value);

		auto power_from = power_from_idx.find(from.value);
		eosio::check(power_from != power_from_idx.end(),"Nothing to delegatee");
		eosio::check(power_from -> power > 0, "Nothing to delegate");
		eosio::check(shares > 0, "Delegate amount must be greater then zero");
		eosio::check(shares <= power_from->staked, "Not enough staked power for delegate");
		
		auto dlgtns = delegations.find(reciever.value);
		auto power_to = power_to_idx.find(reciever.value);

		if (dlgtns == delegations.end()){

			delegations.emplace(_me, [&](auto &d){
				d.reciever = reciever;
				d.shares = shares;
			});

		} else {
			delegations.modify(dlgtns, _me, [&](auto &d){
				d.shares += shares;
			});
		};

		//modify power object of sender and propagate votes changes;
		unicore::propagate_votes_changes(host, from, power_from->power, power_from->power - shares);
		
    log_event_with_shares(from, host, power_from->power - shares);

		power_from_idx.modify(power_from, _me, [&](auto &pf){
			pf.staked -= shares;
			pf.power  -= shares;

		});

		//Emplace or modify power object of reciever and propagate votes changes;
		if (power_to == power_to_idx.end()){
			power_to_idx.emplace(_me, [&](auto &pt){
				pt.username = reciever;
				pt.power = shares;
				pt.delegated = shares;		
			});
		} else {
			//modify
			unicore::propagate_votes_changes(host, reciever, power_to->power, power_to->power + shares);
			
      log_event_with_shares(reciever, host, power_from->power + shares);
      
      power_to_idx.modify(power_to, _me, [&](auto &pt){
				pt.power += shares;
				pt.delegated += shares;
			});

			
		}		
	}

  /**
   * @brief      Обновление счетчика голосов
   * Внутренний метод, используемый для обновления голосов у целей в момент покупки/продажи силы. 
   *
   * @param[in]  host       The host
   * @param[in]  voter      The voter
   * @param[in]  old_power  The old power
   * @param[in]  new_power  The new power
   */
	void unicore::propagate_votes_changes(eosio::name host, eosio::name voter, uint64_t old_power, uint64_t new_power){
		votes_index votes(_me, voter.value);
		goals_index goals(_me, host.value);

		//by host;
		auto idx = votes.template get_index<"host"_n>();
    auto matched_itr = idx.lower_bound(host.value);
       

    while(matched_itr != idx.end() && matched_itr->host == host){
			auto goal = goals.find(matched_itr -> goal_id);
				
			goals.modify(goal, _me, [&](auto &g){
				g.total_votes = goal->total_votes - old_power + new_power;
			});

			idx.modify(matched_itr, _me, [&](auto &v) {
				v.power = v.power - old_power + new_power;
			});
			
      matched_itr++;

		};	
	}



  /**
   * @brief      Метод возврата делегированной силы
   *
   * @param[in]  op    The operation
   */
	[[eosio::action]] void unicore::undelshares(eosio::name from, eosio::name reciever, eosio::name host, uint64_t shares){
		require_auth(reciever);

		power3_index power_from_idx (_me, host.value);
		power3_index power_to_idx (_me, host.value);

		delegation_index delegations(_me, reciever.value);
		auto dlgtns = delegations.find(from.value);

		eosio::check(dlgtns != delegations.end(), "Nothing to undelegate");
		eosio::check(dlgtns -> shares >= shares, "Not enought shares for undelegate");
		eosio::check(shares > 0, "Undelegate amount must be greater then zero");
		
		auto power_from = power_from_idx.find(from.value);
		auto power_to = power_to_idx.find(reciever.value);

		if (dlgtns->shares - shares == 0){
			delegations.erase(dlgtns);
		} else {
			delegations.modify(dlgtns, reciever, [&](auto &d){
				d.shares -= shares;
			});
		}

		//modify power object of sender and propagate votes changes;
		unicore::propagate_votes_changes(host, from, power_from->power, power_from->power - shares);
		log_event_with_shares(from, host, power_from->power - shares);

		power_from_idx.modify(power_from, reciever, [&](auto &pf){
			pf.power -= shares;
			pf.delegated -= shares;

		});

		//modify
		unicore::propagate_votes_changes(host, reciever, power_to->power, power_to->power + shares);
		log_event_with_shares(reciever, host, power_from->power + shares);

		power_to_idx.modify(power_to, reciever, [&](auto &pt){
			pt.staked += shares;
			pt.power  += shares;
		});

		
		

	}

  /**
   * @brief      Публичный метод продажи силы рынку за котировочный токен
   *
   * @param[in]  op    The operation
   */
	[[eosio::action]] void unicore::sellshares(eosio::name username, eosio::name host, uint64_t shares){
		require_auth(username);
		
    account_index accounts(_me, host.value);
    auto exist = accounts.find(host.value);
    
		power3_index power(_me, host.value);
		auto userpower = power.find(username.value);
		auto upower = (userpower->staked);
		eosio::check(upower >= shares, "Not enought power available for sell");

		market_index market(_me, host.value);
		auto itr = market.find(0);
		auto tmp = *itr;

		eosio::asset tokens_out;
		market.modify( itr, _me, [&]( auto& es ) {
        	tokens_out = es.direct_convert( asset(shares,eosio::symbol(eosio::symbol_code("POWER"), 0)), exist -> quote_amount.symbol);
	    });
		eosio::check( tokens_out.amount > 1, "token amount received from selling shares is too low" );
	    
	    make_vesting_action(username, host, exist -> quote_token_contract, tokens_out, itr->vesting_seconds, "powersell"_n);
	    
	    unicore::propagate_votes_changes(host, username, userpower->power, userpower-> power - shares);
      log_event_with_shares(username, host, userpower->power - shares);

	    power.modify(userpower, username, [&](auto &p){
	    	p.power = userpower->power - shares;
	    	p.staked = userpower->staked - shares;
	    });

      // unicore::checkminpwr(host, username);
    
	    
	};

  /**
   * @brief      Disable power market
   *
   * @param[in]  host    The host
   */
  
  [[eosio::action]] void unicore::dispmarket(eosio::name host){
    require_auth(host);
    
    account_index account(_me, host.value);

    auto acc = account.find(host.value);
    eosio::check(acc != account.end(), "Host is not found");

    eosio::check(acc->power_market_id != ""_n, "Power market already disabled");

    account.modify(acc, host, [&](auto &a){
      a.power_market_id = ""_n;
    });
    
  }

  /**
   * @brief      Enable power market
   *
   * @param[in]  host    The host
   * @param[in]  status  The status
   */
  [[eosio::action]] void unicore::enpmarket(eosio::name host){
    require_auth(host);
    
    account_index account(_me, host.value);

    auto acc = account.find(host.value);
    eosio::check(acc != account.end(), "Host is not found");

    eosio::check(acc->power_market_id == ""_n, "Power market already enabled");

    account.modify(acc, host, [&](auto &a){
      a.power_market_id = host;
    });
    
  }

  /**
   * @brief      Приватный метод создания банкор-рынка.
   *
   * @param[in]  host          The host
   * @param[in]  total_shares  The total shares
   * @param[in]  quote_amount  The quote amount
   */
	void unicore::create_bancor_market(std::string name, uint64_t id, eosio::name host, uint64_t total_shares, eosio::asset quote_amount, eosio::name quote_token_contract, uint64_t vesting_seconds){
		market_index market(_me, host.value);
		auto itr = market.find(id);
		if (itr == market.end()){
			itr = market.emplace( _me, [&]( auto& m ) {
               m.id = id;
               m.vesting_seconds = vesting_seconds;
               m.name = name;
               m.supply.amount = 100000000000000ll;
               m.supply.symbol = eosio::symbol(eosio::symbol_code("BANCORE"), 0);
               m.base.balance.amount = total_shares;
               m.base.balance.symbol = eosio::symbol(eosio::symbol_code("POWER"), 0);

               m.quote.balance.amount = quote_amount.amount;
               m.quote.balance.symbol = quote_amount.symbol;
               m.quote.contract = quote_token_contract;
            });
		} else {
			// eosio::check(false, "Market already created");
    }
	};
// };