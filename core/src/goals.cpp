
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

  /**
   * @brief      Метод создания цели
   * 
   * @param[in]  op    The new value
   */
	[[eosio::action]] void unicore::setgoal(eosio::name creator, eosio::name host, uint64_t parent_id, std::string title, std::string description, eosio::asset target, std::string meta){
		require_auth(creator);
		
		goals_index goals(_me, host.value);

    if (parent_id > 0){
      auto parent_goal = goals.find(parent_id);
      eosio::check(parent_goal != goals.end(), "Parent goal is not found");
    };

		account_index accounts(_me, _me.value);

		auto acc = accounts.find(host.value);
    auto root_symbol = (acc->root_token).symbol;

    eosio::check(acc -> type != "fraction"_n, "Fraction host mode prohibit simple set goal method");
    
    eosio::check(target.symbol == root_symbol, "Wrong symbol for this host");
    
    // eosio::check(target.amount > 0, "Target should be more then zero");

    // eosio::check(title.length() <= 300 && title.length() > 0, "Short Description is a maximum 300 symbols. Describe the goal shortly");
    
    uint64_t id = get_global_id("goals"_n);
    
    uint64_t min_amount = unicore::getcondition(host, "mingamount");
    uint64_t min_percent = unicore::getcondition(host, "mingpercent");
    eosio::asset min_asset_amount = asset(0, target.symbol);
    
    if (min_amount > 0) {
      
      min_asset_amount = asset(min_amount, target.symbol);
      unicore::sub_balance(creator, min_asset_amount, acc -> root_token_contract);
    
    } else if (min_percent > 0) {

      min_asset_amount = asset(target.amount * min_percent / HUNDR_PERCENT, target.symbol);
      unicore::sub_balance(creator, min_asset_amount, acc -> root_token_contract);

    }

    uint64_t max_target = unicore::getcondition(host, "maxtarget");
    
    eosio::check(target.amount <= max_target, "Target limit is exceeded. Try to decrease target amount");
  
    eosio::name status = creator == host ? "process"_n :  "waiting"_n;

    if (acc -> consensus_percent > 0)
      status = "waiting"_n;

    goals.emplace(creator, [&](auto &g){
      g.id = id;
      g.parent_id = parent_id;
      g.creator = creator;
      // g.benefactor = creator;
      g.status = status;
      g.created = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
      g.finish_at = eosio::time_point_sec(-1);
      g.host = host;
      g.title = title;
      g.description = description;
      g.target = target;
      g.target1 = asset(0, acc->asset_on_sale.symbol);
      g.target2 = asset(0, acc->asset_on_sale.symbol);
      g.second_circuit_votes = 0;
      g.debt_amount = asset(0, root_symbol);
      g.withdrawed = asset(0, root_symbol);
      g.available = min_asset_amount;
      g.meta = meta;
    });      

    accounts.modify(acc, creator, [&](auto &a) {
      a.total_goals = acc -> total_goals + 1;
    });  


    // action(
    //   permission_level{ _me, "active"_n },
    //   _me, "vote"_n,
    //   std::make_tuple( creator, host, id, true) 
    // ).send();   

    print("GOAL_ID:", id);

	};



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

  /**
   * @brief      Метод создания цели
   * 
   * @param[in]  op    The new value
   */
  [[eosio::action]] void unicore::editgoal(eosio::name editor, eosio::name host, uint64_t goal_id, std::string title, std::string description, std::string meta){
    require_auth(editor);
    
    goals_index goals(_me, host.value);
    account_index accounts(_me, _me.value);

    auto acc = accounts.find(host.value);
    auto goal = goals.find(goal_id);

    eosio::check(goal != goals.end(), "Goal is not found");

    eosio::check(goal ->creator == editor || goal -> benefactor == editor || acc -> architect == editor, "Only creator, benefactor or architect of the goal can edit the goal");

    goals.modify(goal, editor, [&](auto &g){
      g.title = title;
      g.description = description;
      g.meta = meta;
    });      
    
  };

  /**
   * @brief      Метод прямого финансирования цели
   * Используется архитектором для финансирования цели из фонда 
   *
   * @param[in]  op    The operation
   */

  [[eosio::action]] void unicore::setbenefac(eosio::name host, uint64_t goal_id, eosio::name benefactor){
    
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);

    require_auth(acc -> architect);
    
    goals_index goals(_me, host.value);

    auto goal = goals.find(goal_id);

    eosio::check(goal != goals.end(), "goal is not found");

    goals.modify(goal, acc -> architect, [&](auto &g) {
      g.benefactor = benefactor;
    });

  }

  /**
   * @brief      Метод прямого финансирования цели
   * Используется архитектором для финансирования цели из фонда 
   *
   * @param[in]  op    The operation
   */

	[[eosio::action]] void unicore::dfundgoal(eosio::name architect, eosio::name host, uint64_t goal_id, eosio::asset amount, std::string comment){
		require_auth(architect);

		// account_index accounts(_me, _me.value);
		// auto acc = accounts.find(host.value);
		// eosio::check(acc->architect == architect, "Only architect can direct fund the goal");

  //   emission_index emis(_me, host.value);
  //   auto emi = emis.find(host.value);

  //   eosio::check(emi->fund >= amount, "Not enough tokens for fund the goal");

  //   donate_action(architect, host, goal_id, amount, _me);

  //   emis.modify(emi, architect, [&](auto &e){
  //   	e.fund = e.fund - amount;
  //   });

	};


  /**
   * @brief      Метод прямого финансирования родительской цели
   * Используется архитектором для наполнения дочерней цели за счет баланса родительской цели
   *
   * @param[in]  op    The operation
   */

  [[eosio::action]] void unicore::fundchildgoa(eosio::name architect, eosio::name host, uint64_t goal_id, eosio::asset amount){
    require_auth(architect);

    account_index accounts(_me, _me.value);
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
    
    eosio::check(goal->status == ""_n || goal -> status == "waiting"_n, "Only not accepted goals can be accepted");

    goals.modify(goal, host, [&](auto &g) {
      g.benefactor = status == true ? host : ""_n;
      g.status = status == true ? "process"_n : "declined"_n;
      g.parent_id = parent_goal_id;
    });

    if (parent_goal_id != 0 && status == true) {
      
      auto parent_goal = goals.find(parent_goal_id);
      eosio::check(parent_goal != goals.end(), "Parent batch is not founded");
      
    };


  }



  [[eosio::action]] void unicore::gpause(eosio::name host, uint64_t goal_id) {
    //accept or any
    require_auth(host);
    goals_index goals(_me, host.value);
    
    auto goal = goals.find(goal_id);
    eosio::check(goal != goals.end(), "Goal is not exist");
    
    goals.modify(goal, host, [&](auto &g) {
      g.status = "pause"_n;
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

    // if (goal ->parent_id != 0){
    //   auto parent_goal = goals.find(goal->parent_id);

    //   if (parent_goal != goals.end()){

    //     std::vector<uint64_t> batch = parent_goal -> batch;
          
    //     auto itr = std::find(batch.begin(), batch.end(), goal_id);
    //     if (itr != batch.end())
    //       batch.erase(itr);

    //     goals.modify(parent_goal, username, [&](auto &g){
    //       g.batch = batch;
    //     });
    //   }
    // }

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
	
  	account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
    auto root_symbol = acc->get_root_symbol();

		eosio::check(acc != accounts.end(), "Host is not found");

		auto goal = goals.find(goal_id);

    eosio::check("process"_n == goal -> status || "filled"_n == goal->status || "validated"_n == goal -> status, "Only processed goals can be completed");
    
		eosio::check(username == goal->benefactor, "Only benefactor can set report");
		

		goals.modify(goal, username, [&](auto &g) {
      g.status = "reported"_n;  
      g.finish_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + _VOTING_TIME_EXPIRATION);
      g.available = asset(0, goal -> available.symbol);
      g.report = report;
      g.total_asset_on_distribution = goal->available;
      g.remain_asset_on_distribution = goal -> available;
      g.second_circuit_votes += _START_VOTES;
		});

    reports_index reports(_me, host.value);

    reports.emplace(username, [&](auto &r) {
      r.report_id = get_global_id("reports"_n);
      r.task_id = 0;
      r.goal_id = goal -> id;
      r.count = 1;
      r.username = username;
      r.data = report;
      r.need_check = false;
      r.approved = true;
      r.requested = asset(0, root_symbol);;
      r.balance = asset(0, root_symbol);
      r.withdrawed = asset(0, root_symbol);
      r.curator = host;
      r.positive_votes = _START_VOTES;
      r.expired_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch() + 30 * 86400);
      r.created_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch());
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
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		goals_index goals(_me, host.value);
		auto goal = goals.find(goal_id);
		eosio::check(goal != goals.end(), "Goal is not found");
		eosio::check(architect == acc->architect, "Only architect can check report for now");
		eosio::check(goal->status == "reported"_n, "Goals without reports cannot be checked");

		goals.modify(goal, architect, [&](auto &g) {
			g.status = "completed"_n;
      g.finish_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + _VOTING_TIME_EXPIRATION);
		});

    accounts.modify(acc, architect, [&](auto &a) {
      a.achieved_goals = acc -> achieved_goals + 1;
    });

    // uint64_t votes = goal -> positive_votes > 0 ? goal -> positive_votes : (goal -> filled_votes > 0 ? goal -> filled_votes : 1) ;

    // uint64_t weight = votes * goal -> priority;

    //TODO 
    // add to distribution
    // action(
    //   permission_level{ _me, "active"_n },
    //   "auction"_n, "addlbrobj"_n,
    //   std::make_tuple( host , goal -> benefactor, "goal"_n, goal -> id, weight) 
    // ).send();
    

	};





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
      account_index hosts (_me, _me.value);
      auto host = hosts.find(hostname.value);
      eosio::check(host != hosts.end(), "Host not exist");
      
      auto ahost = host->get_ahost();
      auto root_symbol = host->get_root_symbol();
  
      emission_index emis(_me, hostname.value);
      auto emi = emis.find(hostname.value);
      eosio::check(gtop <= 100, "Goal top should be less then 100");
      eosio::check(percent <= 1000 * ONE_PERCENT, "Emission percent should be less then 1000 * ONE_PERCENT");
      
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
		
    eosio::check(goal -> benefactor == username, "Only coordinator can withdraw goal funds");

		account_index accounts(_me, _me.value);
		auto acc = accounts.find((goal->host).value);
		eosio::check(acc != accounts.end(), "Host is not founded");

		auto root_symbol = (acc->root_token).symbol;
    // eosio::check(goal -> status == "filled"_n, "Only filled goals can be withdrawed");
    // eosio::check(acc->direct_goal_withdraw == true, "Direct withdraw from goal is prohibited.");
		eosio::check(goal->creator == username, "You are not owner of this goal");
		eosio::check((goal->available).amount > 0, "Cannot withdraw a zero amount");
    // eosio::check(goal->type == "goal"_n, "Only goal type can be withdrawed by this method");

    
    // eosio::asset cashback = goal->available * 10 / 100;
    eosio::asset on_withdraw = goal->available; // - cashback;

    if (on_withdraw.amount > 0){
    	action(
        permission_level{ _me, "active"_n },
        acc->root_token_contract, "transfer"_n,
        std::make_tuple( _me, goal->benefactor, on_withdraw, std::string("Goal Withdraw")) 
      ).send();

      goals.modify(goal, username, [&](auto &g){
      	g.available = asset(0, root_symbol);
      	g.withdrawed += on_withdraw;
      });
    };

    // if (cashback.amount > 0){
    //   burn_action(username, host, cashback, acc -> root_token_contract);
    // };

	};



    void unicore::spread_to_benefactors(eosio::name host, eosio::asset amount, uint64_t goal_id){

    }

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
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);

    eosio::name main_host = acc->get_ahost();
    spiral_index spiral(_me, main_host.value);
    auto sp = spiral.find(0);

    rate_index rates(_me, main_host.value);

    auto rate = rates.find(acc -> current_pool_num -1);

    if (code != _me) //For direct donate from fund and by architects action
      eosio::check(acc->root_token_contract == code, "Wrong root token contract for this host");

    eosio::check((acc->root_token).symbol == quantity.symbol, "Wrong root symbol for this host");

    auto goal = goals.find(goal_id);

    eosio::check(goal != goals.end(), "Goal is not exist");

    auto root_symbol = acc->get_root_symbol();

    eosio::asset core_quantity = quantity;
    eosio::asset refs_quantity = asset(0, root_symbol);

    
    if (goal -> cashback > 0) {


      core_quantity = quantity * (HUNDR_PERCENT - goal -> cashback) / HUNDR_PERCENT;

      refs_quantity = quantity - core_quantity;

     
    };

    double power = (double)quantity.amount / (double)acc -> quants_buy_rate * (double)sp -> quants_precision;
    uint64_t user_power = uint64_t(power);

    // action(
    //     permission_level{ _me, "active"_n },
    //     _me, "emitpower"_n,
    //     std::make_tuple( host , from, user_power, false) 
    // ).send();


    goals.modify(goal, _me, [&](auto &g){
      g.available += core_quantity;
      g.withdrawed += refs_quantity;
      g.target1 += asset(user_power, _POWER);
      g.refs_amount += refs_quantity;
    });


    if (refs_quantity.amount > 0) {
      unicore::spread_to_refs(host, from, quantity, refs_quantity, acc -> root_token_contract);
    }

  }


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

