using namespace eosio;
// namespace eosio {
  /**
   * @brief      Создание вестинг-баланса
   * Внутренний метод, используемый при обратном обмене силы сообщества на котировочный токен. 
   * Обеспечивает линейную выдачу жетонов в продолжительности времени. 
   * @param[in]  owner   The owner
   * @param[in]  amount  The amount
   */

	void unicore::make_vesting_action(eosio::name owner, eosio::name host, eosio::name contract, eosio::asset amount, uint64_t vesting_seconds, eosio::name type){
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


  
  void add_base_to_market(eosio::name id, eosio::name host, eosio::asset amount){
      
      market_index market(_me, host.value);
      auto itr = market.find(id.value);
        
      market.modify(itr, _me, [&](auto &m){
        m.base.balance += amount;
      });
  }
  
  void add_quote_to_market(eosio::name id, eosio::name host, eosio::asset amount){
      
      market_index market(_me, host.value);
      auto itr = market.find(id.value);
        
      market.modify(itr, _me, [&](auto &m){
        m.quote.balance += amount;
      });
  }

  void sub_base_from_market(eosio::name id, eosio::name host, eosio::asset amount) {
      
      market_index market(_me, host.value);

      auto itr = market.find(id.value);
      
      eosio::check(itr -> base.balance >= amount, "Cannot substrate base balance");

      market.modify(itr, _me, [&](auto &m){
        m.base.balance -= amount;
      });
  }
  
  void sub_quote_from_market(eosio::name id, eosio::name host, eosio::asset amount){
      
      market_index market(_me, host.value);
      auto itr = market.find(id.value);
      
      eosio::check(itr -> quote.balance >= amount, "Cannot substrate quote balance");

      market.modify(itr, _me, [&](auto &m){
        m.quote.balance -= amount;
      });
  }


  [[eosio::action]] void unicore::emitquote(eosio::name host, eosio::asset amount, eosio::name contract) {
    
    require_auth("eosio"_n);
    account_index account(_me, _me.value);
    auto acc = account.find(host.value);
    eosio::check(acc -> root_token_contract == contract, "Wrong token contract");
    
    add_quote_to_market("power"_n, host, amount);

  };

  void unicore::emittomarket(eosio::name host, eosio::name username, uint64_t user_shares, eosio::asset amount){
    account_index account(_me, _me.value);
    auto acc = account.find(host.value);
    
    spiral_index spiral(_me, host.value);
    auto sp = spiral.find(host.value);

    if (user_shares > 0) {
      market_index market(_me, host.value);
      auto itr = market.find("power"_n.value);
      
      if (itr == market.end()) {
        unicore::create_bancor_market("power"_n, host, 0, eosio::asset(0, amount.symbol), acc -> quote_token_contract, 0); 
      }


      //check balance
      //check is it top-witness or not
      //if not top-witness and system currency - skip
      //or better not make simple transfer, make it by event from system contract
      
      action(
          permission_level{ _me, "active"_n },
          "eosio"_n, "takereserve"_n,
          std::make_tuple( host, acc -> quote_token_contract, amount, std::string("emission for labor")) 
      ).send();

      // action(
      //     permission_level{ _saving, "active"_n },
      //     acc->root_token_contract, "transfer"_n,
      //     std::make_tuple( _saving, _me, asset(user_shares, acc -> quote_amount.symbol), std::string("emission for labor")) 
      // ).send();
      
      add_base_to_market("power"_n, host, asset(user_shares, _POWER));

    };

  }

  /**
   * @brief      Метод эмиссии силы с аукциона
   * 
   * @param[in]  op    The operation
   */
  [[eosio::action]] void unicore::emitquants(eosio::name host, eosio::name username, int64_t quants, bool is_change){
    eosio::check(has_auth(_me), "missing required authority");
    
    account_index account(_me, _me.value);
    auto acc = account.find(host.value);
    
    if (quants < 0)
      eosio::check(acc -> total_quants >= abs(quants), "System overflow");

    if (is_change == false)
      account.modify(acc, _me, [&](auto &a){
        a.total_quants += quants;
      });

    quants_index power(_me, host.value);
    auto pexist = power.find(username.value);
    

    if (pexist == power.end()) {

      power.emplace(_me, [&](auto &p) {
        p.username = username;
        p.power = quants;
      });

       unicore::log_event_with_shares(username, host, 0, quants);
        
    } else {
        // unicore::propagate_votes_changes(host, username, pexist->power, pexist->power + user_shares);
        unicore::log_event_with_shares(username, host, 0, pexist->power + quants);
        
        power.modify(pexist, _me, [&](auto &p) {
            p.power += quants;
        });
    };


  }



  /**
   * @brief      Метод эмиссии силы с аукциона на цель
   * 
   * @param[in]  op    The operation
   */

  [[eosio::action]] void unicore::emitpower2(eosio::name host, uint64_t goal_id, uint64_t shares) {
    eosio::check(has_auth("auction"_n) || has_auth(_me), "missing required authority");

    // account_index account(_me, _me.value);
    // auto acc = account.find(host.value);
    
    // // eosio::check(acc -> sale_mode == "auction"_n, "Wrong sale type");
    
    // action(
    //     permission_level{ _saving, "active"_n },
    //     acc->root_token_contract, "transfer"_n,
    //     std::make_tuple( _saving, _me, asset(shares, acc -> quote_amount.symbol), std::string("")) 
    // ).send();


    // add_base_to_market("power"_n, host, asset(shares, _POWER));
    // add_quote_to_market("power"_n, host, asset(shares, acc -> quote_amount.symbol));

    // account.modify(acc, _me, [&](auto &a){
    //   a.total_shares += shares;
    //   a.quote_amount += asset(shares, acc -> quote_amount.symbol);
    // });

    // goals_index goals(_me, host.value);
    // auto goal = goals.find(goal_id);
    // goals.modify(goal, _me, [&](auto &g){
    //   g.total_power_on_distribution += shares;
    //   g.remain_power_on_distribution += shares;
    // });
  }



  /**
   * @brief      Публичный метод продажи силы рынку за котировочный токен
   *
   * @param[in]  op    The operation
   */
  [[eosio::action]] void unicore::sellshares(eosio::name username, eosio::name host, uint64_t shares){
    require_auth(username);
    
    // power3_index power(_me, host.value);
    // auto pexist = power.find(username.value);
    
    // account_index account(_me, _me.value);
    // auto acc = account.find(host.value);


    // uint64_t max_power_on_sale = pexist -> staked; 

    // eosio::check(max_power_on_sale >= shares, "Cannot back more power then you have");

    // if (pexist != power.end() && pexist -> staked > 0) {
      
      
    //   // double available = (double)shares / (double)acc -> total_shares * (double)acc->quote_amount.amount;
    //   // eosio::asset available_asset = asset((uint64_t)available, acc -> quote_amount.symbol);

    //   market_index market(_me, host.value);
    //   auto itr = market.find("power"_n.value);
    //   auto tmp = *itr;

    //   eosio::asset tokens_out;

    //   market.modify( itr, _me, [&]( auto& es ) {
    //      tokens_out = es.direct_convert( asset(shares,eosio::symbol(eosio::symbol_code("POWER"), 0)), acc -> quote_amount.symbol);
    //   });


    //   print("on sell shares: ", shares);
      
    //   power.modify(pexist, username, [&](auto &p){
    //     p.power -= shares;
    //     p.staked -= shares;
    //   });


    //  if (tokens_out.amount > 0) {
    //   //TODO precision?!
    //     if (itr -> vesting_seconds == 0) {

    //       action(
    //           permission_level{ _me, "active"_n },
    //           acc->root_token_contract, "transfer"_n,
    //           std::make_tuple( _me, username, tokens_out, std::string("PWITHDRAW-" + (name{username}.to_string() + "-" + (name{host}).to_string()) ))
    //       ).send();

    //     } else make_vesting_action(username, host, acc -> quote_token_contract, tokens_out, itr -> vesting_seconds, "powersell"_n);
      
    //  } else eosio::check(false, "Token amount is too low");


    // }

    //   account_index accounts(_me, _me.value);
    //   auto exist = accounts.find(host.value);
      
      // power3_index power(_me, host.value);
      // auto userpower = power.find(username.value);
      // auto upower = (userpower->staked);
      // eosio::check(upower >= shares, "Not enought power available for sell");

      // market_index market(_me, host.value);
      // auto itr = market.find(0);
      // auto tmp = *itr;

      // eosio::asset tokens_out;
      // market.modify( itr, _me, [&]( auto& es ) {
    //        tokens_out = es.direct_convert( asset(shares,eosio::symbol(eosio::symbol_code("POWER"), 0)), exist -> quote_amount.symbol);
     //    });

      // eosio::check( tokens_out.amount > 1, "token amount received from selling shares is too low" );
       
    //   uint64_t vesting_seconds = unicore::getcondition(host, "vestingsecs");

    //   make_vesting_action(username, host, exist -> quote_token_contract, tokens_out, vesting_seconds, "powersell"_n);
      
    //   unicore::propagate_votes_changes(host, username, userpower->power, userpower-> power - shares);
    //   log_event_with_shares(username, host, userpower->power - shares);

    //   power.modify(userpower, username, [&](auto &p){
    //    p.power = userpower->power - shares;
    //    p.staked = userpower->staked - shares;
    //   });

        // unicore::checkminpwr(host, username);
  
  };


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
    account_index account(_me, _me.value);
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
  [[eosio::action]] void unicore::withpbenefit(eosio::name username, eosio::name host, uint64_t log_id){
      account_index accounts(_me, _me.value);
      auto acc = accounts.find(host.value);
      auto main_host = acc -> get_ahost();
      powerstat_index powerstats(_me, host.value);
      powerlog_index powerlogs(_me, host.value);
 
      auto powerlog = powerlogs.find(log_id);
      eosio::check(powerlog != powerlogs.end(), "Power log is not founded for current username");
      eosio::check(powerlog -> updated == true, "Only updated balance can be withdrawed");
      // require_auth(powerlog -> username);


      // require_auth(powerlog -> username);
      eosio::check(has_auth(powerlog -> username) || has_auth("eosio"_n), "missing required authority");
    
      eosio::name payer;

      if (has_auth(powerlog -> username)) {
          payer = powerlog -> username;
      } else if (has_auth("eosio"_n))
          payer = "eosio"_n;



      auto powerstat = powerstats.find(powerlog -> window_id);
      
      eosio::check(powerstat != powerstats.end(), "system error: window is not found");
      eosio::check(powerlog -> username == username, "username on action and plog should equal");

      eosio::check(powerstat -> window_closed_at.sec_since_epoch() < eosio::current_time_point().sec_since_epoch(), "Window is not closed yet.");
      
      
      powerstats.modify(powerstat, payer, [&](auto &ps) {
          ps.total_distributed = powerstat -> total_distributed + powerlog -> available_by_loyality + powerlog -> available_by_quants;
      });


      if (powerlog -> available_by_loyality.amount > 0) {
          
        std::string memo = std::string("вывод выгоды по токенам лояльности") + std::string(" из DAO ") + host.to_string(); 

        unicore::push_to_dacs_funds(powerlog->username, powerlog -> available_by_loyality, acc -> root_token_contract, memo);

      } 


      if (powerlog -> available_by_quants.amount > 0 ) {

        std::string memo = std::string("вывод выгоды по токенам центра") + std::string(" из DAO ") + host.to_string(); 

        unicore::push_to_dacs_funds(powerlog->username, powerlog -> available_by_quants, acc -> root_token_contract, memo);

      }

      powerlogs.erase(powerlog);  
          

  }
  




  /**
   * @brief      Метод обновления силового финансового потока
   * 
   * Позволяет обновить доступную часть финансового потока, направленного на держателя силы, 
   * а так же собрать доступные реферальные балансы в один объект
  */
  [[eosio::action]] void unicore::refreshpu(eosio::name username, eosio::name host, uint64_t log_id) {
      account_index accounts(_me, _me.value);
      auto acc = accounts.find(host.value);

      powerstat_index powerstats(_me, host.value);
      powerlog_index powerlogs(_me, host.value);
 
      
      auto powerlog = powerlogs.find(log_id);
      
      eosio::check(powerlog != powerlogs.end(), "Power log is not founded for current username");

      // require_auth(powerlog -> username);
      eosio::check(has_auth(powerlog -> username) || has_auth("eosio"_n), "missing required authority");
    
      eosio::name payer;

      if (has_auth(powerlog -> username)) {
          payer = powerlog -> username;
          eosio::check(powerlog -> username == username, "username on action and plog should equal");
      } else if (has_auth("eosio"_n))
          payer = "eosio"_n;


      if (powerlog -> window_id > 0) {
        auto user_power_by_window_idx = powerlogs.template get_index<"userwindowid"_n>();
        auto prev_log_ids = combine_ids(username.value, powerlog -> window_id);

        auto prev_log = user_power_by_window_idx.begin();
        
        print("now_log: ", powerlog -> id);
        print("prev_log: ", prev_log -> id);

        if (prev_log -> username == username) {
          eosio::check(prev_log -> id == powerlog -> id, "Cannot refresh object with previous objects. Refresh previous first.");
        }
        
      };


      auto powerstat = powerstats.find(powerlog -> window_id);
      
      eosio::check(powerstat != powerstats.end(), "system error: window is not found");
      
      eosio::check(powerstat -> window_closed_at.sec_since_epoch() < eosio::current_time_point().sec_since_epoch(), "Window is not closed yet.");
      

      if (powerlog -> updated == false){ //not updated
        //TODO MODIFY IT

        // double share = double(powerlog -> power) / double(powerstat -> liquid_power);
        // eosio::asset user_amount = asset(share * powerstat -> total_available.amount, powerstat -> total_available.symbol);
        
        // eosio::asset ref_user_amount = asset(share * powerstat -> total_partners_available.amount, powerstat -> total_partners_available.symbol);

        // eosio::check(powerstat -> total_remain >= user_amount, "system error: not enough funds on the window");

        // powerstats.modify(powerstat, payer, [&](auto &ps){
        //     ps.total_remain = powerstat -> total_remain - user_amount;
        //     ps.total_partners_distributed += ref_user_amount;
        // });

        // if (ref_user_amount.amount > 0) {
        //   unicore::spread_to_refs(host, powerlog -> username, ref_user_amount, powerstat -> total_partners_available, acc -> root_token_contract);  
        // };
        

        // powerlogs.modify(powerlog, payer, [&](auto &ps){
        //     ps.available = user_amount;
        //     ps.distributed_to_partners = ref_user_amount;
        //     ps.updated = true;
        // });


        // //get next log and push new if not exist
        // powerstat++;
        
        // eosio::check(powerstat != powerstats.end(), "Next window is not opened yet");

        // auto user_power_by_window_idx = powerlogs.template get_index<"userwindowid"_n>();
        // auto next_log_ids = combine_ids(username.value, powerstat -> id);
        // auto next_log = user_power_by_window_idx.find(next_log_ids);

        // if (next_log == user_power_by_window_idx.end()) {

        //   powerlogs.emplace(payer, [&](auto &pl){
        //       pl.id = unicore::get_global_id("powerlog"_n);
        //       pl.host = host;
        //       pl.username = username;
        //       pl.window_id = powerstat -> id;
        //       pl.power = powerlog -> power;
        //       pl.available = asset(0, powerlog -> available.symbol);
        //       pl.distributed_to_partners = asset(0, powerlog -> available.symbol);
        //       pl.updated = false;
        //     });
        // };
      };
      
  }


  /**
   * @brief      Внутренний метод логирования событий покупки силы
   * Используется для расчета доли владения в финансовом потоке cfund в рамках границы пула
   *
  */
   void unicore::log_event_with_shares (eosio::name username, eosio::name host, int64_t loyality_power, int64_t quants_power) {
     
      account_index accounts(_me, _me.value);
      auto acc = accounts.find(host.value);

      auto root_symbol = acc -> get_root_symbol();

      powerstat_index powerstats(_me, host.value);
      powerlog_index powerlogs(_me, host.value);

      auto first_window = powerstats.find(0);
      
      if (first_window != powerstats.end()) {

        uint64_t now_secs = eosio::current_time_point().sec_since_epoch();
        uint64_t first_window_start_secs = first_window -> window_open_at.sec_since_epoch();
        uint64_t cycle = (now_secs - first_window_start_secs) / _WINDOW_SECS;

        auto current_window = powerstats.find(cycle);
        
        if (current_window != powerstats.end()) {
            
            powerstats.modify(current_window, _me, [&](auto &ps) {
                ps.liquid_quants = acc -> total_quants;
                ps.liquid_loyality = acc -> total_loyality;
            });
       
            auto user_power_by_window_idx = powerlogs.template get_index<"userwindowid"_n>();
            auto log_ids = combine_ids(username.value, cycle);
            
            auto log = user_power_by_window_idx.find(log_ids);
            
            if (log == user_power_by_window_idx.end()) {
              //emplace
              powerlogs.emplace(_me, [&](auto &pl) {
                pl.id = unicore::get_global_id("powerlog"_n);
                pl.host = host;
                pl.username = username;
                pl.window_id = cycle;
                pl.loyality_power = loyality_power;
                pl.quants_power = quants_power;
                pl.available_by_quants = asset(0, root_symbol);
                pl.available_by_loyality = asset(0, root_symbol);
                pl.distributed_to_partners = asset(0, root_symbol);
              });

            } else {
              //modify
              
              user_power_by_window_idx.modify(log, _me, [&](auto &pl){
                pl.loyality_power = log -> loyality_power + loyality_power;
                pl.quants_power = quants_power;
              });
            }


        } else {
          //emplace window if something happen with auto-refresh state

          powerstats.emplace(_me, [&](auto &ps) {
              ps.id = cycle;
              ps.window_open_at = eosio::time_point_sec(first_window_start_secs + _WINDOW_SECS * cycle);
              ps.window_closed_at = eosio::time_point_sec(first_window_start_secs + _WINDOW_SECS * (cycle + 1));
              ps.liquid_quants = acc -> total_quants;
              ps.liquid_loyality = acc -> total_loyality;
              ps.total_available = asset(0, root_symbol);
              ps.total_remain = asset(0, root_symbol);
              ps.total_distributed = asset(0, root_symbol);
              ps.total_partners_distributed = asset(0, root_symbol);
              ps.total_partners_available = asset(0, root_symbol);
          });

          powerlogs.emplace(_me, [&](auto &pl) {
              pl.id = unicore::get_global_id("powerlog"_n);
              pl.host = host;
              pl.username = username;
              pl.window_id = cycle;
              pl.loyality_power = loyality_power;
              pl.quants_power = quants_power;
              pl.available_by_quants = asset(0, root_symbol);
              pl.available_by_loyality = asset(0, root_symbol);
              pl.distributed_to_partners = asset(0, root_symbol);
            });

        }


        
      }

      

      // if (log == pool_idwithhost_idx.end()) {

      //   plogs.emplace(_me, [&](auto &l){
      //     l.id = plogs.available_primary_key();
      //     l.host = host;
      //     l.pool_id = acc->current_pool_id;
      //     l.power = new_power;
      //     l.cycle_num = acc->current_cycle_num;
      //     l.pool_num = acc->current_pool_num;
      //   });

      // } else {
      //   pool_idwithhost_idx.modify(log, _me, [&](auto &l){
      //     l.power = new_power;
      //   });
      // }

      // sincome_index sincome(_me, host.value);
      // auto sinc = sincome.find(acc->current_pool_id);
      
      // rate_index rates(_me, host.value);
      // auto rate = rates.find(acc->current_pool_num - 1);
      // eosio::name main_host = acc->get_ahost();
      // auto root_symbol = acc->get_root_symbol();

      // market_index market(_me, host.value);
      // auto itr = market.find(0);
      // auto liquid_power = acc->total_shares - itr->base.balance.amount;

      // if (sinc == sincome.end()){
      //   sincome.emplace(_me, [&](auto &s){
      //       s.max = rate -> system_income;
      //       s.pool_id = acc->current_pool_id;
      //       s.ahost = main_host;
      //       s.cycle_num = acc->current_cycle_num;
      //       s.pool_num = acc->current_pool_num;
      //       s.liquid_power = liquid_power;
      //       s.total = asset(0, root_symbol);
      //       s.paid_to_sys = asset(0, root_symbol);
      //       s.paid_to_dacs = asset(0, root_symbol);
      //       s.paid_to_cfund = asset(0, root_symbol);
      //       s.paid_to_hfund = asset(0, root_symbol);
      //       s.paid_to_refs = asset(0, root_symbol);
      //       s.hfund_in_segments = 0;
      //       s.distributed_segments = 0;
      //   }); 

      // } else {

      //   sincome.modify(sinc, _me, [&](auto &s){
      //     s.liquid_power = liquid_power;
      //   });
      // } 
      // auto idx = votes.template get_index<"host"_n>();
      // auto i_bv = idx.lower_bound(host.value);


   } 



  /**
   * @brief      Внутренний метод возврата силы при возврате значка.
   * Используется при возврате знака отличия хосту
   *
   * @param[in]  op    The operation
   */

  void unicore::back_shares_with_badge_action (eosio::name host, eosio::name from, uint64_t shares){
    // account_index accounts(_me, _me.value);
    // auto acc = accounts.find(host.value);
    // eosio::check(acc != accounts.end(), "Host is not found");
    
    // power3_index power_to_idx (_me, host.value);
    // auto power_to = power_to_idx.find(from.value);
      
    // //modify
    // unicore::propagate_votes_changes(host, from, power_to->power, power_to->power - shares);

    // // sub_base_from_market(host, asset(shares, _POWER));

    // power_to_idx.modify(power_to, _me, [&](auto &pt){
    //   pt.power -= shares;
    //   pt.with_badges -= shares; 
    // });

    // market_index market(_me, host.value);
    // auto itr = market.find(0);
  
    // accounts.modify(acc, _me, [&](auto &a){
    //   a.total_shares -= shares;
    // });  

  }


  /**
   * @brief      Внутренний метод эмиссии силы.
   * Используется при выдаче знака отличия  
   *
   * @param[in]  op    The operation
   */

  void unicore::give_shares_with_badge_action (eosio::name host, eosio::name reciever, uint64_t shares){
    // account_index accounts(_me, _me.value);
    // auto acc = accounts.find(host.value);
    // eosio::check(acc != accounts.end(), "Host is not found");
    
    // power3_index power_to_idx (_me, host.value);
    // auto power_to = power_to_idx.find(reciever.value);
    
    // market_index market(_me, host.value);
    
    // accounts.modify(acc, _me, [&](auto &a){
    //     a.total_shares += shares;
    // }); 

    // // add_base_to_market(host, asset(shares, _POWER));
      

    // //Emplace or modify power object of reciever and propagate votes changes;
    // if (power_to == power_to_idx.end()) {

    //   power_to_idx.emplace(_me, [&](auto &pt){
    //     pt.username = reciever;
    //     pt.power = shares;
    //     pt.delegated = 0;
    //     pt.with_badges = shares;
    //   });
    
    // } else {
      
    //   //modify
    //   unicore::propagate_votes_changes(host, reciever, power_to->power, power_to->power + shares);
      
      
    //   power_to_idx.modify(power_to, _me, [&](auto &pt){
    //     pt.power += shares;
    //     pt.with_badges += shares; 
    //   });      
    // }   
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
			
      if (goal -> status == "waiting"_n || goal -> status == "validated"_n){
      	goals.modify(goal, _me, [&](auto &g){
  				g.positive_votes = goal->positive_votes - old_power + new_power;
  			});
      }

			idx.modify(matched_itr, _me, [&](auto &v) {
				v.power = v.power - old_power + new_power;
			});
			
      matched_itr++;

		};	
	}


  /**
   * @brief      Disable power market
   *
   * @param[in]  host    The host
   */
  
  [[eosio::action]] void unicore::dispmarket(eosio::name host){
    require_auth(host);
    
    account_index account(_me, _me.value);

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
    
    // account_index account(_me, _me.value);

    // auto acc = account.find(host.value);
    // eosio::check(acc != account.end(), "Host is not found");

    // // eosio::check(acc->power_market_id == ""_n, "Power market already enabled");

    // account.modify(acc, host, [&](auto &a){
    //   a.power_market_id = host;
    // });
    
    // unicore::create_bancor_market("power"_n, host, acc -> total_shares, acc->quote_amount, acc -> quote_token_contract, 0); 
  
  }

  /**
   * @brief      Приватный метод создания банкор-рынка.
   *
   * @param[in]  host          The host
   * @param[in]  total_shares  The total shares
   * @param[in]  quote_amount  The quote amount
   */
	void unicore::create_bancor_market(eosio::name name, eosio::name host, uint64_t total_shares, eosio::asset quote_amount, eosio::name quote_token_contract, uint64_t vesting_seconds){
		market_index market(_me, host.value);

		auto itr = market.find(name.value);
		
    if (itr == market.end()){

			itr = market.emplace( _me, [&]( auto& m ) {
         m.name = name;
         m.vesting_seconds = vesting_seconds;
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






//PREPARE TO DELETE

  /**
   * @brief      Метод делегирования силы.
   * Позволяет делегировать купленную силу для принятия каких-либо решений в пользу любого аккаунта. 
   *
   * @param[in]  op    The operation
   */

  void unicore::delegate_shares_action(eosio::name from, eosio::name reciever, eosio::name host, uint64_t shares){
   //   require_auth(from);
    // power3_index power_from_idx (_me, host.value);
    // power3_index power_to_idx (_me, host.value);

    // delegation_index delegations(_me, from.value);
    
    // account_index accounts(_me, _me.value);
    // auto acc = accounts.find(host.value);

    // auto power_from = power_from_idx.find(from.value);
    // eosio::check(power_from != power_from_idx.end(),"Nothing to delegatee");
    // eosio::check(power_from -> power > 0, "Nothing to delegate");
    // eosio::check(shares > 0, "Delegate amount must be greater then zero");
    // eosio::check(shares <= power_from->staked, "Not enough staked power for delegate");
    
    // auto dlgtns = delegations.find(reciever.value);
    // auto power_to = power_to_idx.find(reciever.value);

    // if (dlgtns == delegations.end()) {

    //  delegations.emplace(_me, [&](auto &d){
    //    d.reciever = reciever;
    //    d.shares = shares;
    //  });

    // } else {
    //  delegations.modify(dlgtns, _me, [&](auto &d){
    //    d.shares += shares;
    //  });
    // };

    // //modify power object of sender and propagate votes changes;
    // unicore::propagate_votes_changes(host, from, power_from->power, power_from->power - shares);
    
  //   log_event_with_shares(from, host, power_from->power - shares);

    // power_from_idx.modify(power_from, _me, [&](auto &pf){
    //  pf.staked -= shares;
    //  pf.power  -= shares;

    // });

    // //Emplace or modify power object of reciever and propagate votes changes;
    // if (power_to == power_to_idx.end()){
    //  power_to_idx.emplace(_me, [&](auto &pt){
    //    pt.username = reciever;
    //    pt.power = shares;
    //    pt.delegated = shares;    
    //  });
    // } else {
    //  //modify
    //  unicore::propagate_votes_changes(host, reciever, power_to->power, power_to->power + shares);
      
  //     log_event_with_shares(reciever, host, power_from->power + shares);
      
  //     power_to_idx.modify(power_to, _me, [&](auto &pt){
    //    pt.power += shares;
    //    pt.delegated += shares;
    //  });

      
    // }    
  }





  /**
   * @brief      Метод покупки силы сообщества
   * Обеспечивает покупку силы сообщества за котировочный токен по внутренней рыночной цене, определяемой с помощью алгоритма банкор. 
   * @param[in]  buyer   The buyer
   * @param[in]  host    The host
   * @param[in]  amount  The amount
   */
  uint64_t unicore::buyshares_action ( eosio::name buyer, eosio::name host, eosio::asset amount, eosio::name code, bool is_frozen ){
    // account_index accounts(_me, _me.value);
    // partners2_index users(_partners,_partners.value);
    // auto user = users.find(buyer.value);
    
    // auto exist = accounts.find(host.value);
    // auto root_symbol = exist -> get_root_symbol();

    // eosio::check(exist != accounts.end(), "Host is not founded");
    // eosio::check(exist -> quote_token_contract == code, "Wrong quote token contract");
    // eosio::check(exist -> quote_amount.symbol == amount.symbol, "Wrong quote token symbol");
    // eosio::check(exist -> power_market_id != ""_n, "Can buy shares only when power market is enabled");

    // eosio::name main_host = exist->get_ahost();

    // spiral_index spiral(_me, main_host.value);
    // auto sp = spiral.find(0);

    // uint64_t new_shares = amount.amount * (HUNDR_PERCENT - sp -> loss_percent) / HUNDR_PERCENT;

    // //TODO buy_shares check!
    // // market.modify( itr, _me, [&]( auto& es ) {
    // //        tokens_out = es.direct_convert( asset(shares,eosio::symbol(eosio::symbol_code("POWER"), 0)), exist -> quote_amount.symbol);
    //  //    });


    // print("new_shares: ", new_shares);
    
    // accounts.modify(exist, _me, [&](auto &a){
    //   a.total_shares += new_shares;
    //   a.quote_amount += amount;
    // });

    // power3_index power(_me, host.value);

    // auto pexist = power.find(buyer.value);
    
    // if (pexist == power.end()) {

    //   power.emplace(_me, [&](auto &p){
    //      p.username = buyer;
    //      p.power = new_shares;
    //      p.staked = new_shares; 
    //   });
      
    //   log_event_with_shares(buyer, host, new_shares);
    
    // } else {
      
    //    unicore::propagate_votes_changes(host, buyer, pexist->power, pexist->power + new_shares);
    //    log_event_with_shares(buyer, host, pexist->power + new_shares);

    //    power.modify(pexist, _me, [&](auto &p){
    //      p.power += new_shares;
    //      p.staked += new_shares;
    //    });
 
    // };

    
    // return amount.amount;
    return 0;
  };



