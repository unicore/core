
using namespace eosio;

/**
 * @brief      Модуль целей. 
 * Использует долю потока эмиссии для финансирования целей сообщества. 
 * Каждый участник может предложить цель к финансированию. Суть и смысл цели ограничен только желаниями сообщества.
 * Каждая цель должна пройти минимальный порог процента собранных голосов от держателей силы сообщества.
 * Цели, прошедшие порог консенсуса сообщества, попадают в лист финансирования. 
 * Все цели в листе финансирования получают равный поток финансирования относительно друг друга.
 * Финансирование поступает в объект баланса цели в момент перехода на каждый следующий пул, или напрямую из фонда сообщества по подписи архитектора.  
 * 
 */

    void validate_permlink( const std::string& permlink )
        {
           eosio::check( 3 < permlink.size() && permlink.size() < 255, "Permlink is not a valid size." );
           
             for( auto c : permlink )
             {
                switch( c )
                {
                   case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
                   case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
                   case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '0':
                   case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                   case '-':
                      break;
                   default:
                      eosio::check( false, "Invalid permlink character");
                }
             }
        }

  /**
   * @brief      Метод создания цели
   * 
   * @param[in]  op    The new value
   */
	[[eosio::action]] void unicore::setgoal(eosio::name creator, eosio::name host, eosio::name type, std::string title, std::string permlink, std::string description, eosio::asset target, uint64_t duration, uint64_t cashback, bool activated, bool is_batch, uint64_t parent_batch_id, std::string meta){
		require_auth(creator);
		
		goals_index goals(_me, host.value);
		account_index accounts(_me, host.value);

		partners2_index users(_partners, _partners.value);
    // auto user = users.find(creator.value);
    // eosio::check(user != users.end(), "User is not registered");
    
    eosio::check(cashback <= 100 * ONE_PERCENT, "Cashback should be less or equal 1000000");

		auto acc = accounts.find(host.value);
    auto root_symbol = (acc->root_token).symbol;

    bool validated = acc->consensus_percent == 0;
    
    validate_permlink(permlink);
    
    //check for uniq permlink
    auto hash = hash64(permlink);
    auto exist_goal = goals.find(hash);
    // auto idx_bv = goals.template get_index<"hash"_n>();
    // auto exist_permlink = idx_bv.find(hash);

    // eosio::check(exist_goal == goals.end(), "Goal with current permlink is already exist");


    eosio::check(target.symbol == root_symbol, "Wrong symbol for this host");
    
    eosio::check(title.length() <= 300 && title.length() > 0, "Short Description is a maximum 100 symbols. Describe the goal shortly");
    
    auto goal_id =  acc -> total_goals + 1;

    auto targets_symbol = acc -> sale_mode == "counterpower"_n ? _POWER : acc->asset_on_sale.symbol;

    if (exist_goal == goals.end()){
      // eosio::check(target.amount == 0, "Goal target is a summ of tasks amounts");
    
      if (parent_batch_id != 0) {
        auto parent_goal = goals.find(parent_batch_id);
        eosio::check(parent_goal != goals.end(), "Parent batch is not founded");
        std::vector<uint64_t> batch = parent_goal -> batch;
        
        //
        //TODO check for batch length
        auto itr = std::find(batch.begin(), batch.end(), hash);
        
        if (itr == batch.end()){
          batch.push_back(hash);
        } 

        goals.modify(parent_goal, creator, [&](auto &t){
          t.batch = batch;
        });
      };


      if (exist_goal -> parent_id != parent_batch_id){
        auto parent_goal = goals.find(parent_batch_id);

        if (parent_goal != goals.end()){

          std::vector<uint64_t> batch = parent_goal -> batch;
          
          auto itr = std::find(batch.begin(), batch.end(), exist_goal->id);
          if (itr != batch.end())
            batch.erase(itr);

          goals.modify(parent_goal, creator, [&](auto &g){
            g.batch = batch;
          });
        }
      }
      

      goals.emplace(creator, [&](auto &g){
        g.id = hash;
        g.type = type;
        // g.hash = hash;
        g.creator = creator;
        g.benefactor = creator == host ? host : ""_n;
        g.status = creator == host ? "process"_n : ""_n;
        // g.who_can_create_tasks = creator;
        g.created = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
        g.finish_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + duration);
        g.host = host;
        g.permlink = permlink;
        g.cashback = cashback;
        g.title = title;
        g.description = description;
        g.target = target;
        g.filled = target.amount == 0 ? true : false;
        g.target1 = asset(0, targets_symbol);
        g.target2 = asset(0, targets_symbol);
        g.target3 = asset(0, targets_symbol);
        g.debt_amount = asset(0, root_symbol);
        g.withdrawed = asset(0, root_symbol);
        g.available = asset(0, root_symbol);
        g.validated = validated;
        g.duration = duration;
        g.meta = meta;
        g.activated = activated;
        g.is_batch = is_batch;
        g.parent_id = parent_batch_id;
      });      

      accounts.modify(acc, creator, [&](auto &a){
        a.total_goals = acc -> total_goals + 1;
      });  

    } else {

      goals.modify(exist_goal, creator, [&](auto &g){
        g.who_can_create_tasks = creator;
        g.host = host;
        g.permlink = permlink;
        g.parent_id = parent_batch_id;
        g.cashback = cashback;
        g.title = title;
        g.description = description;
        g.target = target;
        g.validated = validated;
        g.duration = duration;
        g.meta = meta;
        g.activated = activated;
        g.filled = exist_goal->available + exist_goal->withdrawed >= target; 
      });

    }



      // if (creator == acc->architect && type == "goal"_n) {
      //   unicore::dfundgoal(creator, host, goal_id, target, "autoset by architect");
      // }


	};


  /**
   * @brief      Метод прямого финансирования цели
   * Используется архитектором для финансирования цели из фонда 
   *
   * @param[in]  op    The operation
   */

	[[eosio::action]] void unicore::dfundgoal(eosio::name architect, eosio::name host, uint64_t goal_id, eosio::asset amount, std::string comment){
		require_auth(architect);

		account_index accounts(_me, host.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc->architect == architect, "Only architect can direct fund the goal");

    emission_index emis(_me, host.value);
    auto emi = emis.find(host.value);

    eosio::check(emi->fund >= amount, "Not enough tokens for fund the goal");

    donate_action(architect, host, goal_id, amount, _me);

    emis.modify(emi, architect, [&](auto &e){
    	e.fund = e.fund - amount;
    });

	};


  /**
   * @brief      Метод прямого финансирования родительской цели
   * Используется архитектором для наполнения дочерней цели за счет баланса родительской цели
   *
   * @param[in]  op    The operation
   */

  [[eosio::action]] void unicore::fundchildgoa(eosio::name architect, eosio::name host, uint64_t goal_id, eosio::asset amount){
    require_auth(architect);

    account_index accounts(_me, host.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc->architect == architect, "Only architect can direct fund the goal");

    goals_index goals(_me, host.value);
    auto child_goal = goals.find(goal_id);
    auto parent_goal = goals.find(child_goal -> parent_id);
    eosio::check(parent_goal != goals.end(), "Parent goal is not found");
    eosio::check(parent_goal -> available >= amount, "Not enough parent balance for fund");

    goals.modify(parent_goal, architect, [&](auto &gp){
      gp.available -= amount;
      gp.withdrawed += amount;
    });

    goals.modify(child_goal, architect, [&](auto &gc){
      gc.available += amount;
    });

  };



 
  [[eosio::action]] void unicore::setgcreator(eosio::name oldcreator, eosio::name newcreator, eosio::name host, uint64_t goal_id){
    require_auth(oldcreator);
    goals_index goals(_me, host.value);
    
    auto goal = goals.find(goal_id);
    eosio::check(goal != goals.end(), "Goal is not exist");
    eosio::check(goal->creator == oldcreator, "Wrong creator of goal");
    
    goals.modify(goal, oldcreator, [&](auto &g){
      g.creator = newcreator;
      g.who_can_create_tasks = newcreator;
    });

  }


 
  [[eosio::action]] void unicore::gaccept(eosio::name host, uint64_t goal_id, uint64_t parent_goal_id, bool status) {
    //accept or any
    require_auth(host);
    goals_index goals(_me, host.value);
    
    auto goal = goals.find(goal_id);
    eosio::check(goal != goals.end(), "Goal is not exist");
    
    goals.modify(goal, host, [&](auto &g) {
      g.benefactor = status == true ? host : ""_n;
      g.status = status == true ? "process"_n : "declined"_n;
      g.parent_id = parent_goal_id;
    });

    if (goal->is_batch == false){
      eosio::check(parent_goal_id != 0, "Accepted goal should have a parent");
    }

    if (parent_goal_id != 0 && status == true) {
      
      auto parent_goal = goals.find(parent_goal_id);
      eosio::check(parent_goal != goals.end(), "Parent batch is not founded");
      std::vector<uint64_t> batch = parent_goal -> batch;
      
      //TODO check for batch length
      auto itr = std::find(batch.begin(), batch.end(), goal_id);
      
      if (itr == batch.end()){
        batch.push_back(goal_id);
      };

      goals.modify(parent_goal, host, [&](auto &t){
        t.batch = batch;
      });
    };


  }



  [[eosio::action]] void unicore::gpause(eosio::name host, uint64_t goal_id) {
    //accept or any
    require_auth(host);
    goals_index goals(_me, host.value);
    
    auto goal = goals.find(goal_id);
    eosio::check(goal != goals.end(), "Goal is not exist");
    
    goals.modify(goal, host, [&](auto &g) {
      g.status = ""_n;
    });
  }



  /**
   * @brief      Метод удаления цели
   *
   * @param[in]  op    The operation
   */
	[[eosio::action]] void unicore::delgoal(eosio::name username, eosio::name host, uint64_t goal_id){
		require_auth(username);
		goals_index goals(_me, host.value);
    
		auto goal = goals.find(goal_id);
		eosio::check(goal != goals.end(), "Goal is not exist");
    eosio::check(goal -> total_tasks == 0, "Impossible to delete goal with tasks. Clear tasks first");
    eosio::check(goal->creator == username || username == host, "Wrong auth for del goal");
    eosio::check(goal->debt_count == 0, "Cannot delete goal with the debt");

    if (goal ->parent_id != 0){
      auto parent_goal = goals.find(goal->parent_id);

      if (parent_goal != goals.end()){

        std::vector<uint64_t> batch = parent_goal -> batch;
          
        auto itr = std::find(batch.begin(), batch.end(), goal_id);
        if (itr != batch.end())
          batch.erase(itr);

        goals.modify(parent_goal, username, [&](auto &g){
          g.batch = batch;
        });
      }
    }

		goals.erase(goal);

	}

  /**
   * @brief      Метод создания отчета о завершенной цели
   *
   * @param[in]  op    The operation
   */
	[[eosio::action]] void unicore::report(eosio::name username, eosio::name host, uint64_t goal_id, std::string report){
		require_auth(username);
		
		goals_index goals(_me, host.value);
	
  	account_index accounts(_me, host.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		auto goal = goals.find(goal_id);

    eosio::check("process"_n == goal->status, "Only processed goals can be completed");
    
		eosio::check(username == goal->benefactor, "Only benefactor can set report");
		

		goals.modify(goal, username, [&](auto &g) {
      // if (goal->available.amount == 0) {
        g.status = "reported"_n;  
      // } else {
      //   g.checked = true;
      //   g.status = "completed"_n;  
      //   accounts.modify(acc, architect, [&](auto &a) {
      //     a.achieved_goals = acc -> achieved_goals + 1;
      //   });
      // }
			
      g.report = report;
      g.reported = true;

		});
	}

  /**
   * @brief      Метод проверки отчета
   * Отчет о достижении цели на текущий момент проверяется только архитектором. 
   *
   * @param[in]  op    The operation
   */
	[[eosio::action]] void unicore::check(eosio::name architect, eosio::name host, uint64_t goal_id){
		require_auth(architect);
		account_index accounts(_me, host.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		goals_index goals(_me, host.value);
		auto goal = goals.find(goal_id);
		eosio::check(goal != goals.end(), "Goal is not found");
		eosio::check(architect == acc->architect, "Only architect can eosio::check report for now");
		eosio::check(goal->reported == true, "Goals without reports cannot be eosio::checked");

		goals.modify(goal, architect, [&](auto &g){
			g.checked = true;
      g.status = "completed"_n;
		});

    accounts.modify(acc, architect, [&](auto &a){
      a.achieved_goals = acc -> achieved_goals + 1;
    });
	};

  /**
   * @brief      Метод спонсорского взноса на цель
   * Позволяет любому участнику произвести финансирование цели из числа собственных средств. 
   *
   * @param[in]  from      The from
   * @param[in]  host      The host
   * @param[in]  goal_id   The goal identifier
   * @param[in]  quantity  The quantity
   * @param[in]  code      The code
   */
	void unicore::donate_action(eosio::name from, eosio::name host, uint64_t goal_id, eosio::asset quantity, eosio::name code){
		// require_auth(from);
		
		goals_index goals(_me, host.value);
		account_index accounts(_me, host.value);
		auto acc = accounts.find(host.value);
    spiral_index spiral(_me, host.value);
    auto sp = spiral.find(0);

    rate_index rates(_me, host.value);

    auto rate = rates.find(acc -> current_pool_num -1);
		if (code != _me) //For direct donate from fund and by architects action
			eosio::check(acc->root_token_contract == code, "Wrong root token contract for this host");
		
		eosio::check((acc->root_token).symbol == quantity.symbol, "Wrong root symbol for this host");
		
		auto goal = goals.find(goal_id);
		
		eosio::check(goal != goals.end(), "Goal is not exist");

		bool filled = goal->available + goal->withdrawed + quantity >= goal->target;		
		
    auto root_symbol = acc->get_root_symbol();

    if (acc -> sale_is_enabled == true && from != host) {

      if (goal->type=="marathon"_n) {
        
          eosio::check(goal->target <= quantity, "Wrong quantity for join the marathon");

          goalspartic_index gparticipants(_me, host.value);
          
          auto users_with_id = gparticipants.template get_index<"byusergoal"_n>();

          auto goal_ids = combine_ids(from.value, goal_id);
          auto participant = users_with_id.find(goal_ids);

          // eosio::check(participant == users_with_id.end(), "Username already participate in the current marathon");
          
          if (participant != users_with_id.end()){
            gparticipants.emplace(_me, [&](auto &p){
              p.id = gparticipants.available_primary_key();
              p.goal_id = goal_id;
              p.role = "participant"_n;
              p.username = from;
              p.expiration = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + 30 * 86400);
            });

            goals.modify(goal, _me, [&](auto &g){
              g.participants_count += 1;
              g.withdrawed += quantity;
            });
          }
        }

        auto targets_symbol = acc -> sale_mode == "counterpower"_n ? _POWER : acc->asset_on_sale.symbol;

        eosio::asset target1 = goal -> target1;
        eosio::asset target2 = goal -> target2;

        eosio::asset converted_quantity;

        eosio::asset core_quantity = quantity * (HUNDR_PERCENT - goal -> cashback) / HUNDR_PERCENT;
        
        eosio::asset refs_quantity = quantity - core_quantity;

        pool_index pools(_me, host.value);

        auto pool = pools.find(acc->current_pool_id);

        print(" quantity: ", quantity);
        print(" core_quantity: ", core_quantity);
        print(" refs_quantity: ", refs_quantity);

        if (core_quantity.amount >= pool -> quant_cost.amount){
          
          if (acc -> sale_mode == "counterpower"_n) {

            converted_quantity = unicore::buy_action(from, host, core_quantity, acc->root_token_contract, false, false, false);
            uint64_t shares_out = unicore::buyshares_action ( from, host, converted_quantity, acc->quote_token_contract, false );
            target2= asset(goal->target2.amount + shares_out, _POWER);
          
          } else if (acc -> sale_mode == "counterunit"_n) {

            converted_quantity = unicore::buy_action(from, host, core_quantity, acc->root_token_contract, false, true, false);
            target1 = asset(goal->target1.amount + converted_quantity.amount, acc->asset_on_sale.symbol);
          
          }
          
        
        } else {
          
          refs_quantity += core_quantity;
          core_quantity.amount = 0;
          print(" refs_quantity: ", refs_quantity);

        }

        
        goals.modify(goal, _me, [&](auto &g){
          g.available += quantity;
          g.withdrawed += refs_quantity;

          g.filled = filled;
          g.target1 = target1;
          g.target2= target2;
        });


        if (refs_quantity.amount > 0) {
          unicore::spread_to_refs(host, from, quantity, refs_quantity);
        }

      
    } else {


        goals.modify(goal, _me, [&](auto &g){
          g.available += quantity;
          g.filled = filled;
        });

    }


	}

  /**
   * @brief      Метод финансирования цели через оператора сообщества.
   * Позволяет оператору сообщества расходовать баланс целей на перечисления прямым спонсорам. 
   * Используется в риверсной экономической модели, когда корневой токен сообщества является котировочным токеном силы сообщества, 
   * и накаким другим способом изначально не распределяется, кроме как на спонсоров целей (дефицитная ICO - модель).
   *
   * @param[in]  op    The operation
   */
  [[eosio::action]] void unicore::gsponsor(eosio::name hoperator, eosio::name host, eosio::name reciever, uint64_t goal_id, eosio::asset amount){
    require_auth (hoperator);

    // account_index accounts(_me, host.value);
    // auto acc = accounts.find(host.value);
    // eosio::check(acc->hoperator == hoperator, "Wrong operator for this host");
    // goals_index goals(_me, host.value);

    // auto goal = goals.find(goal_id);
    // eosio::check(goal != goals.end(), "Goal is not founded");
    // auto root_symbol = (acc->root_token).symbol;

    // eosio::check(amount.symbol == root_symbol, "Wrong root symbol");

    // eosio::check(goal->available >= amount, "Not enough tokens for sponsorhip action. ");

    // action(
    //   permission_level{ _me, "active"_n },
    //   acc->root_token_contract, "transfer"_n,
    //   std::make_tuple( _me, reciever, amount, std::string("Sponsor")) 
    // ).send();

    // goals.modify(goal, hoperator, [&](auto &g){
    //   g.available = g.available - amount;
    //   g.withdrawed = g.withdrawed + amount;
    // });

  }





/**
 * @brief Метод выплаты долга по цели из числа available в счет объектов долга
 *       
 *
 * @param[in]  op    The new value
 */

  [[eosio::action]] void unicore::paydebt(eosio::name host, uint64_t goal_id){
      require_auth(host);

      goals_index goals(_me, host.value);
      auto goal = goals.find(goal_id);
      eosio::check(goal != goals.end(), "Goal is not found");

      account_index hosts (_me, host.value);
      auto acc = hosts.find(host.value);
      eosio::check(acc != hosts.end(), "Host not exist");
      
      debts_index debts(_me, host.value);

      auto idx_bv = debts.template get_index<"bygoal"_n>();
      auto debt = idx_bv.begin();
      
      uint64_t count = 0;
      uint64_t limit = 10;
      
      std::vector<uint64_t> list_for_delete;
      eosio::asset available = goal -> available;
      eosio::asset debt_amount = goal -> debt_amount;
      print("imher1");
      while(debt != idx_bv.end() && count != limit && debt -> goal_id == goal_id) {
        print("imher2");
        if (available >= debt -> amount ){        
          print("imher3");
          list_for_delete.push_back(debt->id);
          available -= debt-> amount;     
          debt_amount -= debt -> amount;

          action(
            permission_level{ _me, "active"_n },
            acc->root_token_contract, "transfer"_n,
            std::make_tuple( _me, debt->username, debt->amount, std::string("Debt Withdraw")) 
          ).send();   

        };

        count++;
        debt++;

      };

      for (auto id : list_for_delete){
        auto debt_for_delete = debts.find(id);

        debts.erase(debt_for_delete);
      };

      goals.modify(goal, host, [&](auto &g){
        g.available = available;
        g.debt_amount = debt_amount;
      });
  };


/**
 * @brief Метод установки скорости эмиссии и размера листа финансирования
 *       
 *
 * @param[in]  op    The new value
 */
   [[eosio::action]] void unicore::setemi(eosio::name hostname, uint64_t percent, uint64_t gtop){
      require_auth(hostname);
      account_index hosts (_me, hostname.value);
      auto host = hosts.find(hostname.value);
      eosio::check(host != hosts.end(), "Host not exist");
      
      auto ahost = host->get_ahost();
      auto root_symbol = host->get_root_symbol();
  
      emission_index emis(_me, hostname.value);
      auto emi = emis.find(hostname.value);
      eosio::check(gtop <= 100, "Goal top should be less then 100");
      eosio::check(percent <= 1000 * ONE_PERCENT, "Emission percent should be less then 100 * ONE_PERCENT");
      
      emis.modify(emi, hostname, [&](auto &e){
          e.percent = percent;
          e.gtop = gtop;
      });
    }


  /**
   * @brief      Метод вывода баланса цели
   * Выводит доступный баланс цели на аккаунт создателя цели.
   *
   * @param[in]  op    The operation
   */
	[[eosio::action]] void unicore::gwithdraw(eosio::name username, eosio::name host, uint64_t goal_id){
		require_auth(username);
		
		goals_index goals(_me, host.value);
		auto goal = goals.find(goal_id);
		
		eosio::check(goal != goals.end(), "Goal is not founded");
		
		account_index accounts(_me, (goal->host).value);
		auto acc = accounts.find((goal->host).value);
		eosio::check(acc != accounts.end(), "Host is not founded");

		auto root_symbol = (acc->root_token).symbol;
    eosio::check(acc->direct_goal_withdraw == true, "Direct withdraw from goal is prohibited.");
		eosio::check(goal->creator == username, "You are not owner of this goal");
		eosio::check((goal->available).amount > 0, "Cannot withdraw a zero amount");
    eosio::check(goal->type == "goal"_n, "Only goal type can be withdrawed by this method");

    eosio::asset on_withdraw = goal->available;
    	
  	action(
      permission_level{ _me, "active"_n },
      acc->root_token_contract, "transfer"_n,
      std::make_tuple( _me, goal->creator, on_withdraw, std::string("Goal Withdraw")) 
    ).send();

    goals.modify(goal, username, [&](auto &g){
    	g.available = asset(0, root_symbol);
    	g.withdrawed += on_withdraw;
    });

	};




    [[eosio::action]] void unicore::withdrbeninc(eosio::name username, eosio::name host, uint64_t goal_id){
        require_auth(username);
        benefactors_index benefactors(_me, host.value);

        // auto benefactor = benefactors.find(username.value);

        auto benefactor_with_goal_id = benefactors.template get_index<"bybengoal"_n>();
        auto benefactor_ids = combine_ids(username.value, goal_id);
        auto benefactor = benefactor_with_goal_id.find(benefactor_ids);

        eosio::check(benefactor != benefactor_with_goal_id.end(), "Benefactor is not found");
        
        account_index accounts(_me, host.value);
        auto acc = accounts.find(host.value);

        auto root_symbol = acc->get_root_symbol();
        auto income = benefactor->income;
        eosio::check(income.amount > 0, "Only positive amount can be withdrawed");

        action(
            permission_level{ _me, "active"_n },
            acc->root_token_contract, "transfer"_n,
            std::make_tuple( _me, username, income, std::string("benefactor income")) 
        ).send();

        benefactor_with_goal_id.modify(benefactor, username, [&](auto &d){
            d.income = asset(0, root_symbol);
            d.income_in_segments = 0;
            d.withdrawed += income;
        });

    }

    [[eosio::action]] void unicore::addben(eosio::name creator, eosio::name username, eosio::name host, uint64_t goal_id, uint64_t weight) {
        require_auth(creator);
        
        account_index accounts(_me, host.value);
        benefactors_index benefactors(_me, host.value);

        auto benefactor_with_goal_id = benefactors.template get_index<"bybengoal"_n>();
        auto benefactor_ids = combine_ids(username.value, goal_id);
        auto benefactor = benefactor_with_goal_id.find(benefactor_ids);
        eosio::check(benefactor == benefactor_with_goal_id.end(), "Benefactor is already exist");

        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Host is not found");
        auto root_symbol = acc->get_root_symbol();
        
        goals_index goals(_me, host.value);
        auto goal = goals.find(goal_id);
        
        eosio::check(creator == goal->creator, "Only goal creator can modify benefactors table");
        
        eosio::check(goal != goals.end(), "Marathon is not found");

        benefactors.emplace(creator, [&](auto &d){
            d.id = benefactors.available_primary_key();
            d.goal_id = goal_id;
            d.benefactor = username;
            d.weight = weight;
            d.income = asset(0, root_symbol);
            d.income_in_segments = 0;
            d.withdrawed = asset(0, root_symbol);
            d.income_limit = asset(0, root_symbol);
        });

        
        goals.modify(goal, creator, [&](auto &h){
            h.benefactors_weight += weight;
        });
    };

    [[eosio::action]] void unicore::rmben(eosio::name creator, eosio::name username, eosio::name host, uint64_t goal_id){
        require_auth(creator);
        account_index accounts(_me, host.value);
        benefactors_index benefactors(_me, host.value);

        auto benefactor_with_goal_id = benefactors.template get_index<"bybengoal"_n>();
        auto benefactor_ids = combine_ids(username.value, goal_id);
        auto benefactor = benefactor_with_goal_id.find(benefactor_ids);

        eosio::check(benefactor != benefactor_with_goal_id.end(), "Benefactor is not found");

        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Host is not found");

        goals_index goals(_me, host.value);
        auto goal = goals.find(goal_id);
        eosio::check(goal != goals.end(), "Marathon is not found");
        eosio::check(goal->creator == creator, "You are not creator of the goal for set benefactors");
        
        goals.modify(goal, creator, [&](auto &h){
            h.benefactors_weight -= benefactor->weight;
        });
        
        if (benefactor->income.amount > 0)
          action(
              permission_level{ _me, "active"_n },
              acc->root_token_contract, "transfer"_n,
              std::make_tuple( _me, username, benefactor->income, std::string("Benefactor income before remove")) 
          ).send();

        benefactor_with_goal_id.erase(benefactor);
    };


    void unicore::spread_to_benefactors(eosio::name host, eosio::asset amount, uint64_t goal_id){

        benefactors_index benefactors(_me, host.value);
        account_index accounts(_me, host.value);
        auto acc = accounts.find(host.value);
        auto root_symbol = acc->get_root_symbol();

        eosio::check(amount.symbol == root_symbol, "System error on spead to dacs");
        
        goals_index goals(_me, host.value);
        auto goal = goals.find(goal_id);
        eosio::check(goal != goals.end(), "Marathon is not found");

        
        auto benefactor_with_goal_id = benefactors.template get_index<"bygoalid"_n>();
        auto benefactor = benefactor_with_goal_id.lower_bound(goal_id);
        print("benefactor_id", benefactor -> id);

        if (benefactor != benefactor_with_goal_id.end() && benefactor -> goal_id == goal_id){
            while(benefactor != benefactor_with_goal_id.end() && benefactor -> goal_id == goal_id) {
                
                eosio::asset amount_for_benefactor = asset(amount.amount * benefactor -> weight / goal-> benefactors_weight, root_symbol);
                
                benefactor_with_goal_id.modify(benefactor, _me, [&](auto &d){

                    uint128_t dac_income_in_segments = amount_for_benefactor.amount * TOTAL_SEGMENTS;

                    double income = double(benefactor ->income_in_segments + dac_income_in_segments) / (double)TOTAL_SEGMENTS;

                    d.income = asset(uint64_t(income), root_symbol);
                    d.income_in_segments += amount_for_benefactor.amount * TOTAL_SEGMENTS;
                
                });

                benefactor++;

            }
        }  else {
            
            benefactors.emplace(_me, [&](auto &d){
                d.id = benefactors.available_primary_key();
                d.goal_id = goal_id;
                d.benefactor = host;
                d.weight = 1;
                d.income = amount;
                d.income_in_segments = amount.amount * TOTAL_SEGMENTS;
                d.withdrawed = asset(0, root_symbol);
                d.income_limit = asset(0, root_symbol);
            });

            goals.modify(goal, _me, [&](auto &h){
                h.benefactors_weight = 1;
            });
        }

    }

