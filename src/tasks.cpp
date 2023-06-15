
[[eosio::action]] void unicore::setinctask(eosio::name username, uint64_t income_id, bool with_badge, uint64_t my_goal_id, uint64_t my_badge_id) {
	require_auth(username);

	incoming_index incoming (_me, username.value);

	auto inc = incoming.find(income_id);

	if (inc != incoming.end()){

		goals_index goals(_me, username.value);
		
		if (my_goal_id != 0){
			auto goal = goals.find(my_goal_id);
			eosio::check(goal != goals.end(), "User goal id not found");	
		}
		

		badge_index badges(_me, username.value);
		auto badge = badges.find(my_badge_id);
		eosio::check(badge != badges.end(), "User badge is not found ");

		incoming.modify(inc, username, [&](auto &i){
			i.with_badge = with_badge;
			i.my_goal_id = my_goal_id;
			i.my_badge_id = my_badge_id;
		});	
	}
	


};
	
void unicore::check_and_gift_netted_badge(eosio::name username, eosio::name host, uint64_t task_id){

	incoming_index incoming (_me, username.value);


	auto host_with_task_index = incoming.template get_index<"byhosttask"_n>();
	auto task_idx = combine_ids(host.value, task_id);
	auto is_exist = host_with_task_index.find(task_idx);


	if (is_exist != host_with_task_index.end() && is_exist -> with_badge) {
		if (is_exist -> my_goal_id == 0){
			unicore::giftbadge_action(username, username, is_exist->my_badge_id, std::string("Completed task"), false, false, 0,0);
		} else {
			unicore::giftbadge_action(username, username, is_exist->my_badge_id, std::string("Completed task"), true, false, is_exist->my_goal_id, 0);
		}
	};

	
	

}	
 

 void unicore::setincoming(eosio::name doer, eosio::name host, uint64_t goal_id, uint64_t task_id) {
 	
 	incoming_index incoming (_me, doer.value);

 	eosio::check(goal_id == 0, "If set task as incoming, goal_id is should be zero");

	auto host_with_task_index = incoming.template get_index<"byhosttask"_n>();
	auto task_idx = combine_ids(host.value, task_id);
	auto is_exist = host_with_task_index.find(task_idx);

	if (is_exist == host_with_task_index.end()){
		incoming.emplace(_me, [&](auto &d){
			d.id = incoming.available_primary_key();
			d.host = host;
			d.task_id = task_id;
			d.goal_id = goal_id;
		});
	}
 	// if (goal_id > 0) {
 	// 	eosio::check(task_id == 0, "If set goal as incoming, task_id is should be zero");

	 // 	auto host_with_goal_index = incoming.template get_index<"byhostgoal"_n>();
		// auto goal_idx = combine_ids(host.value, task_id);
		// auto is_exist = host_with_goal_index.find(goal_idx);

		// if (is_exist == host_with_goal_index.end()) {
		// 	incoming.emplace(_me, [&](auto &d){
		// 		d.id = incoming.available_primary_key();
		// 		d.host = host;
		// 		d.task_id = task_id;
		// 		d.goal_id = goal_id;
		// 	});
		// }


 	// } else if (task_id > 0) {
 	// 	eosio::check(goal_id == 0, "If set task as incoming, goal_id is should be zero");

 	// 	auto host_with_task_index = incoming.template get_index<"byhosttask"_n>();
		// auto task_idx = combine_ids(host.value, task_id);
		// auto is_exist = host_with_task_index.find(task_idx);

		// if (is_exist == host_with_task_index.end()){
		// 	incoming.emplace(_me, [&](auto &d){
		// 		d.id = incoming.available_primary_key();
		// 		d.host = host;
		// 		d.task_id = task_id;
		// 		d.goal_id = goal_id;
		// 	});
		// }


 	// } else if (task_id == 0 && goal_id == 0){
 	// 	eosio::check(false, "Goal and task ids cannot be a zero both");
 	// };



 }


 void unicore::delincoming(eosio::name doer, eosio::name host, uint64_t goal_id, uint64_t task_id) {
 	incoming_index incoming (_me, doer.value);

 	if (goal_id > 0) {
 		eosio::check(task_id == 0, "If set goal as incoming, task_id is should be zero");

	 	auto host_with_goal_index = incoming.template get_index<"byhostgoal"_n>();
		auto goal_idx = combine_ids(host.value, goal_id);
		auto is_exist = host_with_goal_index.find(goal_idx);

		if (is_exist != host_with_goal_index.end()) {
			host_with_goal_index.erase(is_exist);
		}

 	} else if (task_id > 0) {
 		eosio::check(goal_id == 0, "If set task as incoming, goal_id is should be zero");

 		auto host_with_task_index = incoming.template get_index<"byhosttask"_n>();
		auto task_idx = combine_ids(host.value, task_id);
		auto is_exist = host_with_task_index.find(task_idx);

		if (is_exist != host_with_task_index.end()) {
			host_with_task_index.erase(is_exist);
		}


 	} else if (task_id == 0 && goal_id == 0){
 			eosio::check(false, "Goal and task ids cannot be a zero both");
 	};


 }

/**
 * @brief      Модуль задач
 * Задачи есть составляющие части достижения любой цели. 
 * Постановка задач осуществляется только в рамках целей. 
 * Задачи могут быть 
 * 	- публичными, т.е. достуными к исполнению каждым участником сообщества, 
 * 	- приватными, т.е. доступными к исполнению только внутренним управляющим командам.
 */

	/**
	 * @brief      Публичный метод создания задачи
	 * Может использовать только аккаунт хоста на текущий момент. 
	 *
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::settask(eosio::name host, eosio::name creator, std::string permlink, uint64_t goal_id, uint64_t priority, eosio::string title, eosio::string data, eosio::asset requested, bool is_public, eosio::name doer, eosio::asset for_each, bool with_badge, uint64_t badge_id, bool is_batch, uint64_t parent_batch_id, bool is_regular, std::vector<uint64_t> calendar, eosio::time_point_sec start_at,eosio::time_point_sec expired_at, std::string meta){
		
		eosio::check(has_auth(creator) || has_auth(host), "missing required authority");
    
    auto payer = has_auth(creator) ? creator : host;

		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		auto root_symbol = acc->get_root_symbol();
		
		eosio::check(requested.amount == 0 , "Requested amount should be zero now");
		
		if (is_public == true) {
			eosio::check(requested.symbol == root_symbol, "Wrong token for current host" );
			eosio::check(for_each.symbol == root_symbol, "Wrong token for current host" );
		} else {
			eosio::check(for_each.amount == 0, "Wrong amount for current host and task mode" );
		}
		
		
    tasks_index tasks(_me, host.value);
		uint64_t id = get_global_id("tasks"_n);


		if (with_badge == true) {
			badge_index badges(_me, host.value);
			auto badge = badges.find(badge_id);
			eosio::check(badge != badges.end(), "Badge with current ID is not found");
		}


		if (parent_batch_id != 0) {
			auto parent_task = tasks.find(parent_batch_id);
			eosio::check(parent_task != tasks.end(), "Parent batch is not founded");
		};

		goals_index goals(_me, host.value);
		auto goal = goals.find(goal_id);
		eosio::check(goal != goals.end(), "Goal is not found");

		if (doer != ""_n) {
			eosio::check( is_account( doer ), "user account does not exist");
			auto doer_host = accounts.find(doer.value);
			bool doer_is_host = doer_host == accounts.end() ? false: true;
			doers_index doers(_me, host.value);
			
			// auto doers_with_tasks_index = doers.template get_index<"doerwithtask"_n>();
			// auto task_ids = combine_ids(doer.value, hash);
			// auto is_exist = doers_with_tasks_index.find(task_ids);
			// if (is_exist == doers_with_tasks_index.end()){
				
			doers.emplace(payer,[&](auto &d){
				d.id = get_global_id("doers"_n);
				d.task_id = id;
				d.doer = doer;
				d.is_host = doer_is_host;
				d.doer_goal_id = 0;
				d.doer_badge_id = 0;
				d.comment = "";
			});

			// } else {

			// 	doers_with_tasks_index.modify(is_exist, payer, [&](auto &d){
			// 		d.doer = doer;
			// 		d.is_host = doer_is_host;
			// 	});

			// }
		} 

		
		bool validated = creator == acc->architect ? true : false;	
		
		//todo check calendar for 7 days
		//TODO add incoming goal for doer			
		if (doer != host && doer != ""_n) {
			setincoming(doer, host, 0, id);	
		};


		tasks.emplace(payer, [&](auto &t){
			t.id = id;
			t.parent_id = parent_batch_id;
			t.goal_id = goal_id;	
			t.type = goal != goals.end() ? goal -> type : "goal"_n;
			t.host = host;
			t.creator = creator;
			t.title = title;
			t.data = data;
			t.priority = priority;
			t.permlink = permlink;
			t.requested = asset(0, root_symbol);
			t.for_each = asset(0, root_symbol);
			t.funded = asset(0, root_symbol);
			t.remain = asset(0, root_symbol);
			t.is_public  = is_public;
			t.doer = doer;
			t.is_encrypted = false;
			t.validated = validated;
			t.with_badge = with_badge;
			t.badge_id = badge_id;
			t.duration = 0;
			t.meta = meta;
			t.expired_at = expired_at;
			t.created_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch());
			t.start_at = start_at;
			t.is_regular = is_regular;
			t.calendar = calendar;
		});
		
    accounts.modify(acc, payer, [&](auto &a){
      a.total_tasks = acc -> total_tasks + 1;
    });


    
	// } else {

	// 	eosio::check(exist_task -> status != "completed"_n, "Only not completed tasks can be modified");
		

	// 	goals_index goals(_me, host.value);
	// 	auto goal = goals.find(goal_id);
		
	// 	if (goal != goals.end()){
	// 		eosio::check(goal -> target.symbol == requested.symbol, "Requested symbol and goal symbol should equal");
			
	// 		eosio::asset change = requested - exist_task -> requested;

	// 		goals.modify(goal, payer, [&](auto &g){
	//     	g.target += change;
	//     	g.filled = goal -> available + goal -> withdrawed > goal -> target + change;		
	//     });

	// 	}
		
		
	// 	if (exist_task -> doer != doer && doer != ""_n) {
	// 		delincoming(exist_task -> doer, host, 0, hash);	
	// 		setincoming(doer, host, 0, hash);	
	// 	};


	// 	tasks.modify(exist_task, payer, [&](auto &t){
	// 		t.type = goal != goals.end() ? goal -> type : "goal"_n;

	// 		if (payer == doer && payer != host) {

	// 			t.start_at = start_at;
	// 			t.expired_at = expired_at;
			
	// 		} else {
	// 			t.title = title;
	// 			t.doer = doer;
	// 			t.data = data;
	// 			t.priority = priority;
	// 			t.requested = requested;
	// 			t.for_each = for_each;
	// 			t.is_public = is_public;
	// 			t.with_badge = with_badge;
	// 			t.badge_id = badge_id;
	// 			t.batch = batch;
	// 			t.calendar = calendar;
	// 			t.is_regular = is_regular;
	// 			t.start_at = start_at;
	// 			t.expired_at = expired_at;
	// 			t.meta = meta;
	// 		}
			
	// 	});


	// }

	

	

	// uint64_t task_id = acc -> total_tasks + 1;
	
	

	
  
  // auto idx_bv = goals.template get_index<"hash"_n>();
  // auto exist_permlink = idx_bv.find(hash);

  // eosio::check(exist_task == tasks.end(), "Goal with current permlink is already exist");
  
  //TODO check batch for exist every task



	// if (creator == acc->architect) {
	// 	unicore::fundtask(host, task_id, requested, "fund on set by architect");
	// }
    print("TASK_ID:", id);
	}

	/**
	 * @brief      Публичный метод фондирования задачи
	 * Исполняется хостом для пополнения баланса задачи из доступного баланса цели. 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::fundtask(eosio::name host, uint64_t task_id, eosio::asset amount, eosio::string comment){
		require_auth(host);

		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");
		
		tasks_index tasks(_me, host.value);
		auto task = tasks.find(task_id);
		eosio::check(task != tasks.end(), "Task is not exist");
		goals_index goals(_me, host.value);
		
		auto goal = goals.find(task->goal_id);
		
		eosio::check(goal != goals.end(), "Goal is not found");

		eosio::check(goal->available >= amount, "Not enough tokens for fund in the goal object");

		// eosio::check(task->funded + amount <= task->requested, "Fund amounts is more then requested");

		goals.modify(goal, host, [&](auto &g){
			g.available = goal->available - amount;
			g.withdrawed = goal->withdrawed + amount;
		});

		tasks.modify(task, host, [&](auto &t){
			t.funded = task->funded + amount;
			t.remain = task->remain + amount;
		});

	}

	/**
	 * @brief      Метод удаления задачи
	 * Вызывается хостом для удаления поставленной задачи. 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::deltask(eosio::name host, uint64_t task_id){
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found ");

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);
		
    eosio::check(has_auth(task->creator) || has_auth(task->host), "missing required authority");
    
    auto payer = has_auth(task->creator) ? task->creator : task->host;

		eosio::check(task != tasks.end(), "Task is not found");

		goals_index goals(_me, host.value);
		auto goal = goals.find(task->goal_id);
		
		// eosio::check(goal != goals.end(), "Goal is not found");
		
		// if (goal != goals.end()) {
		// 	goals.modify(goal, payer, [&](auto &g){
	 //    	g.total_tasks -= 1;
	 //    	g.target -= task->requested - task->funded;
	 //    });
		// }

		delincoming(task -> doer, host, 0, task_id);	

		tasks.erase(task);

	}
	


	/**
	 * @brief      Метод установки родительской цели
	 * 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::setpgoal(eosio::name host, uint64_t task_id, uint64_t goal_id){
		require_auth(host);
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");
		
		goals_index goals(_me, host.value);
		auto goal = goals.find(goal_id);
		eosio::check(goal != goals.end(), "Goal is not found");

		eosio::check(task -> requested.symbol == goal -> target.symbol, "Symbol is not equal");

		tasks.modify(task, host, [&](auto &t){
			t.goal_id = goal_id;
		});
	}


	/**
	 * @brief      Метод установки исполнителя
	 * 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::setdoer(eosio::name host, uint64_t task_id, eosio::name doer){
		require_auth(host);
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");
		eosio::check( is_account( doer ), "user account does not exist");
		eosio::check(task -> is_public == false, "Can set doer only for a private tasks");

		
		delincoming(task -> doer, host, 0, task_id);	
		
		if (doer != ""_n && doer != host){
			setincoming(doer, host, 0, task_id);		
		}
		

		tasks.modify(task, host, [&](auto &t){
			t.doer = doer;
		});

	}


	/**
	 * @brief      Метод предложения отклонения заявки деятеля
	 * 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::canceljtask(eosio::name host, uint64_t task_id, eosio::name doer){
		eosio::check(has_auth(doer) || has_auth(host), "missing required authority");
		eosio::name payer = has_auth(doer) ? doer : host;

		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		auto doer_host = accounts.find(doer.value);
		bool doer_is_host = doer_host == accounts.end() ? false: true;

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);
		eosio::check(task != tasks.end(), "Task is not found");

		//report
		reports_index reports(_me, host.value);
		auto users_with_id = reports.template get_index<"userwithtask"_n>();
		auto report_ids = combine_ids(doer.value, task_id);
		auto user_report = users_with_id.find(report_ids);


		eosio::check(user_report == users_with_id.end(), "Cannot cancel doer until delete report");

		if (task -> doer == doer) {

			delincoming(doer, host, 0, task_id);	
		
			tasks.modify(task, payer, [&](auto &t){
				t.doer = ""_n;
			});

		}

		doers_index doers(_me, host.value);
		auto doers_with_tasks_index = doers.template get_index<"doerwithtask"_n>();
		auto task_ids = combine_ids(doer.value, task->id);
		auto is_exist = doers_with_tasks_index.find(task_ids);
		eosio::check(is_exist != doers_with_tasks_index.end(), "Doer is not found");
		
		auto d = doers.find(is_exist -> id);
		doers.erase(d);

	}

	/**
	 * @brief      Метод предложения себя как исполнителя задачи
	 * 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::jointask(eosio::name host, uint64_t task_id, eosio::name doer, std::string comment){
		require_auth(doer);
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		auto doer_host = accounts.find(doer.value);
		bool doer_is_host = doer_host == accounts.end() ? false: true;


		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");

		eosio::check(task -> is_public == false, "Doers is available only in a private tasks");

		doers_index doers(_me, host.value);
		auto doers_with_tasks_index = doers.template get_index<"doerwithtask"_n>();
		auto task_ids = combine_ids(doer.value, task->id);
		auto is_exist = doers_with_tasks_index.find(task_ids);

		if (doer != host)
			setincoming(doer, host, 0, task_id);	

		if (is_exist == doers_with_tasks_index.end()){
			doers.emplace(doer,[&](auto &d){
				d.id = doers.available_primary_key();
				d.task_id = task_id;
				d.doer = doer;
				d.is_host = doer_is_host;
				d.doer_goal_id = 0;
				d.doer_badge_id = 0;
				d.comment = comment;
			});
		} else {
			doers_with_tasks_index.modify(is_exist, doer, [&](auto &d){
				d.comment = comment;
			});
		}
	}

	/**
	 * @brief      Метод ручной валидации задачи
	 * 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::settaskmeta(eosio::name host, uint64_t task_id, std::string meta){
		require_auth(host);
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");

		tasks.modify(task, host, [&](auto &t){
			t.meta = meta;	
		});

	}



	/**
	 * @brief      Метод ручной валидации задачи
	 * 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::setpriority(eosio::name host, uint64_t task_id, uint64_t priority){
		
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");
		require_auth(acc -> architect);

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");
		eosio::check(priority > 0 && priority <= 3, "Priority should be between 0 and 3");

		tasks.modify(task, acc -> architect, [&](auto &t){
			t.priority = priority;
		});

	}
	/**
	 * @brief      Метод ручной валидации задачи
	 * 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::validate(eosio::name host, uint64_t task_id, uint64_t goal_id, eosio::name doer){
		require_auth(host);
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");

		eosio::check(goal_id != 0, "Parent goal is not setted");
		
				

		delincoming(task -> doer, host, 0, task_id);	

		if (doer != ""_n && doer != host){
			eosio::check( is_account( doer ), "user account does not exist");
			setincoming(doer, host, 0, task_id);		
		}
		

		tasks.modify(task, host, [&](auto &t){
			t.validated = true;	
			t.goal_id = goal_id;
			t.doer = doer;
		});

	}

	/**
	 * @brief      Метод активации задачи
	 * Вызывается хостом для активации выполнения поставленной задачи. 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::tactivate(eosio::name host, uint64_t task_id){
		require_auth(host);
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");

		eosio::check(task->active == false, "Task is already active");

		tasks.modify(task, host, [&](auto &t){
			t.active = true;
		});

	}
	
	/**
	 * @brief      Публичный метод деактивации задачи
	 * Применимо для публичных задач, когда поставленная цель достигнута или недостижима.
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::tdeactivate(eosio::name host, uint64_t task_id){
		require_auth(host);
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");

		eosio::check(task->active == true, "Task is already deactivated");

		tasks.modify(task, host, [&](auto &t){
			t.active = false;
		});

	}


	/**
	 * @brief      Публичный метод деактивации задачи
	 * Применимо для публичных задач, когда поставленная цель достигнута или недостижима.
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::tcomplete(eosio::name host, uint64_t task_id){
		require_auth(host);
		account_index accounts(_me, _me.value);
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		tasks_index tasks(_me, host.value);

		auto task = tasks.find(task_id);

		eosio::check(task != tasks.end(), "Task is not found");

		// eosio::check(task-> completed == true, "Task is already completed");

		tasks.modify(task, host, [&](auto &t) {
			t.status = "completed"_n;
			t.completed = true;
			t.active = false;
		});

	}

	/**
	 * @brief      Публичный метод создания отчета о выполненной задаче
	 * Применяется исполнителем задачи для того, чтобы отправить отчет на проверку. 
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::setreport(eosio::name host, eosio::name username, uint64_t task_id, eosio::string data, uint64_t duration_secs, eosio::asset asset_per_hour){
		
		require_auth(username);
		account_index accounts(_me, _me.value);
		partners2_index users(_partners, _partners.value);

		auto acc = accounts.find(host.value);
		auto root_symbol = acc->get_root_symbol();
		
		// eosio::check(root_symbol == requested.symbol, "Wrong token for current host");
		eosio::check(acc != accounts.end(), "Host is not found");

		tasks_index tasks(_me, host.value);
		auto task = tasks.find(task_id);
		eosio::check(task != tasks.end(), "Task is not found");
		eosio::check(task -> active == true, "Task is not active");
		eosio::check(duration_secs > 0, "Duration should be more then zero");

		if (task -> is_public == false){
			eosio::check(task -> doer == username, "Wrong doer for the private task");
		};

		goals_index goals(_me, host.value);
		auto goal = goals.find(task-> goal_id);

		// eosio::check(goal -> status != "reported"_n && goal->status != "completed"_n, "Goal is already completed");
		eosio::check(asset_per_hour.symbol == root_symbol, "Wrong symbol per hour");
		eosio::check(asset_per_hour.amount <= 1000000, "Maximum limit is 100 token per hour for now");

		// if (goal -> type == "marathon"_n) {
		// 	goalspartic_index gparticipants(_me, host.value);
  //     auto users_with_id = gparticipants.template get_index<"byusergoal"_n>();
		// 	auto goal_ids = combine_ids(username.value, task->goal_id);
  //     auto participant = users_with_id.find(goal_ids);

  //     // eosio::check(participant != users_with_id.end(), "Username not participant of the current marathon");
		// };

		
		reports_index reports(_me, host.value);
		
		auto users_with_id = reports.template get_index<"userwithtask"_n>();

		auto report_ids = combine_ids(username.value, task_id);
		auto user_report = users_with_id.find(report_ids);

		// eosio::check(user_report == users_with_id.end(), "Report for this task already exist");
		
		bool need_check = username == host ? false : true;
		uint64_t report_id;

		// if (user_report == users_with_id.end() || task -> is_regular == true){
			eosio::check(task -> priority >= 0, "task priority should be more then zero");

			eosio::asset asset_per_hour2 = (task -> priority == 0 || task -> priority == 1) ? asset(10*10000, root_symbol) : (task -> priority == 2 ? asset(20*10000, root_symbol) : asset(40*10000, root_symbol));
			spiral_index spiral(_me, host.value);
			auto sp = spiral.find(0);

			double req = ((double)sp-> quants_precision * (double)duration_secs * (double)asset_per_hour2.amount / 3600) / (double)acc -> sale_shift;

			eosio::asset requested = task -> is_public == false ? task->requested : asset(req, _POWER);
			eosio::symbol sym = task -> is_public == false ? root_symbol : _POWER;
			report_id = get_global_id("reports"_n);

			reports.emplace(username, [&](auto &r) {
				r.report_id = report_id;
				r.task_id = task_id;
				r.goal_id = task->goal_id;
				r.count = 1;
				r.username = username;
				r.data = data;
				r.need_check = need_check;
				r.requested = requested;
				r.balance = asset(0, root_symbol);
				r.withdrawed = asset(0, root_symbol);
				r.curator = host;
				r.duration_secs = duration_secs;
				r.asset_per_hour = asset_per_hour2;
				r.expired_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch() + 30 * 86400);
				r.created_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch());
			});

			accounts.modify(acc, username, [&](auto &a){
	      a.total_reports = acc -> total_reports + 1;
	    });

		// } else {
		// 	report_id = user_report -> report_id;

		// 	eosio::check(task -> is_regular == true, "Task is not regular, but report is exist");
		// 	eosio::check(user_report -> need_check == false, "Previous report is not checked yet");
			
		// 	users_with_id.modify(user_report, username, [&](auto &r){
		// 		r.count += 1;
		// 		r.need_check = true;
		// 		r.approved = false;
		// 		r.duration_secs += duration_secs;
		// 		r.expired_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch() + 30 * 86400);
		// 		r.created_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch());
		// 		r.asset_per_hour = asset_per_hour;
		// 	});
		// }

		
		goals.modify(goal, _me, [&](auto &g){
			g.total_reports += 1 ;
		});
		
		intelown_index intelown(_me, host.value);
		auto io = intelown.find(username.value);
		
		if (io == intelown.end()) {
			intelown.emplace(username, [&](auto &i){
				i.username = username;
				i.total_reports = 1;
			});
		} else {
			intelown.modify(io, username, [&](auto &i){
				i.total_reports += 1;
			});
		}
		

  //   if (need_check == false) {
  //   	unicore::approver(host, report_id, "");
		// };
		print("REPORT_ID:", report_id);
	}

	/**
	 * @brief      Публиный метод редактирования отчета
	 * В случае, если отчет не принят, участник получает возможность отредактировать свой отчет и выслать его на проверку повторно. 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::editreport(eosio::name host, eosio::name username, uint64_t report_id, eosio::string data){
		// require_auth(voter);
		require_auth(username);
		account_index accounts(_me, _me.value);
		
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		reports_index reports(_me, host.value);
		auto report = reports.find(report_id);

		eosio::check(report->need_check == false, "Cannot modify report until check");

		reports.modify(report, username, [&](auto &r){
			r.need_check = true;
			r.data = data;
			r.comment = "";
		});
	}



	/**
	 * @brief      Публиный метод удаления отчета
	 * В случае, если действие считается завершенным, отчёт может удалить участник или хост
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::delreport(eosio::name host, eosio::name username, uint64_t report_id){
		
		if (!has_auth(host)){
			require_auth(username);
		} else {
			require_auth(host);
		}
		
		account_index accounts(_me, _me.value);
		
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		reports_index reports(_me, host.value);
		auto report = reports.find(report_id);

		tasks_index tasks(_me, host.value);
		auto task = tasks.find(report -> task_id);

		// eosio::check(task -> completed == true, "Only report for a completed task can be removed");
		accounts.modify(acc, host, [&](auto &a){
			a.total_reports -= 1;
		});
		
		reports.erase(report);
	}

	/**
	 * @brief      Публичный метод одобрения отчета
	 * Используется хостом для того, чтобы принять задачу как выполненную и выдать вознаграждение / награду в виде значка. 
	 *
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::approver(eosio::name host, uint64_t report_id, eosio::string comment){
		
		account_index accounts(_me, _me.value);
		
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		reports_index reports(_me, host.value);
		auto report = reports.find(report_id);
		
		tasks_index tasks(_me, host.value);
		auto task = tasks.find(report->task_id);
		
		eosio::check(task != tasks.end(), "Task is not found");
		spiral_index spiral(_me, host.value);
		auto sp = spiral.find(0);
		goals_index goals(_me, host.value);
		auto goal = goals.find(task -> goal_id);
		eosio::check(goal != goals.end(), "Goal is not found");
		
		//AUTH CHECK
		eosio::check(has_auth(acc -> architect) || has_auth(goal -> benefactor), "missing required authority");
    auto payer = has_auth(acc -> architect) ? acc -> architect : goal -> benefactor;
		
		print(4);
		
		if (report -> username != host)
			eosio::check(report->approved == false, "Task is already approved");

		
		if (goal -> status == "process"_n || goal -> status == "validated"_n || goal -> status == "filled"_n || goal -> status == "reported"_n || goal -> status == "completed"_n){

			double asset_per_hour = (double)report -> asset_per_hour.amount * sp -> quants_precision * report -> duration_secs / 3600;
			
			print("asset_per_hour: ", asset_per_hour);

			uint64_t user_power = asset_per_hour / (double)acc -> sale_shift;

			uint64_t bonus_power = user_power * 10 * ONE_PERCENT / HUNDR_PERCENT;
			eosio::asset amount = asset(asset_per_hour / sp -> quants_precision, acc -> quote_amount.symbol);

			print("user_power: ", user_power);
			print("bonus_power: ", bonus_power);
			print("amount for emit: ", amount);

			// action(
	    //     permission_level{ _me, "active"_n },
	    //     _me, "emitpower"_n,
	    //     std::make_tuple( host , report->username, user_power, false) 
	    // ).send();

			//TODO ?
			// action(
	    //     permission_level{ _me, "active"_n },
	    //     _me, "emittomarket"_n,
	    //     std::make_tuple( host , report->username, user_power, amount) 
	    // ).send();
			
			reports.modify(report, payer, [&](auto &r){
				r.need_check = false;
				r.comment = comment;
				r.approved = true;
				r.positive_votes += _START_VOTES;
			});

			goals.modify(goal, payer, [&](auto &g) {
				g.second_circuit_votes += _START_VOTES + report -> positive_votes;
				g.total_power_on_distribution += bonus_power; 
				g.approved_reports += 1;
				g.target1 += asset(user_power + bonus_power, _POWER);
			});

			accounts.modify(acc, payer, [&](auto &a){
				a.approved_reports += 1;
			});

			intelown_index intelown(_me, host.value);
			auto io = intelown.find(report -> username.value);
			
			if (io == intelown.end()) {
				intelown.emplace(payer, [&](auto &i){
					i.username = report->username;
					i.approved_reports = 1;
				});
			} else {
				intelown.modify(io, payer, [&](auto &i){
					i.approved_reports += 1;
				});
			}

		} else {
			eosio::check(false, "Goal is not validated");
			// reports.modify(report, payer, [&](auto &r){
			// 	r.need_check = false;
			// 	r.comment = comment;
			// 	r.approved = true;
			// 	r.distributed = true;
			// });
			
			// goals.modify(goal, payer, [&](auto &g){
			// 	g.approved_reports = 1;
			// })

			// accounts.modify(acc, payer, [&](auto &a){
			// 	a.approved_reports +=1;
			// });

			// intelown_index intelown(_me, host.value);
			// auto io = intelown.find(report -> username);
			
			// intelown.modify(io, payer, [&](auto &i){
			// 	i.approved_reports += 1;
			// })

			//TODO если цель не валидирована, то фракции не распределяем
		}

		// eosio::check(report->requested <= task->remain, "Not enough funds for pay to this task");
		
	// 	if (report->requested.amount > 0) {
	// 		if (report->requested > task->remain) {
	// 			//if available in goal - minus it
	// 			//or modify goal debt which will be payed on the next donation to the debt object
				
				

	// 			if (goal -> available >= report -> requested && goal -> status == "process"_n) {
					
	// 				goals.modify(goal, host, [&](auto &g){
	// 					g.available -= report -> requested;
	// 					g.withdrawed += report -> requested;
	// 					g.approved_reports += 1;
	// 					g.second_circuit_votes += _START_VOTES * task -> priority * report -> duration_secs;
	// 				});

	// 				action(
	//           permission_level{ _me, "active"_n },
	//           acc->root_token_contract, "transfer"_n,
	//           std::make_tuple( _me, report->username, report->requested, comment) 
	// 	      ).send();

	// 			} else {
	// 				//MODIFY NOT CREATE A DEBTS NOW
					
	// 				//TODO use parent_goal first!
	// 				//THEN use CFUND
	// 				//AND THEN CREATE DEBT
					
	// 				goals.modify(goal, host, [&](auto &g){
	// 					// g.available -= report -> requested;
	// 					g.debt_count += 1;
	// 					g.debt_amount += report -> requested;
	// 					g.approved_reports += 1;
	// 				});
					
	// 				debts_index debts(_me, host.value);
					
	// 				debts.emplace(host, [&](auto &d){
	// 					d.id = debts.available_primary_key();
	// 					d.amount = report -> requested;
	// 					d.goal_id = goal->id;
	// 					d.username = report -> username;
	// 				});
				
	// 			}

	// 		} else {
	// 			print("imhere2");

	// 			tasks.modify(task, host, [&](auto &t){
	// 				t.remain = task -> remain - report->requested;
	// 			});

	// 			goals.modify(goal, host, [&](auto &g){
	// 				g.approved_reports += 1;
	// 				g.second_circuit_votes += _START_VOTES * task -> priority * report -> duration_secs;
	// 			});

	// 			action(
 //          permission_level{ _me, "active"_n },
 //          acc->root_token_contract, "transfer"_n,
 //          std::make_tuple( _me, report->username, report->requested, comment) 
	//       ).send();
				
	// 	};

	// } else {

		// goals.modify(goal, host, [&](auto &g){
		// 	g.second_circuit_votes += _START_VOTES * task -> priority * report -> duration_secs;
		// });

	// }



		if (task -> with_badge == true) {
			unicore::giftbadge_action(host, report->username, task->badge_id, std::string("Completed task"), true, true, task->goal_id, report->task_id);

			// //Выдаём значок создателю действия, если предусмотрено
			// uint64_t creator_badge_id = unicore::getcondition(host, "creatorbadge");
			
			// if (creator_badge_id > 0){

			// 	unicore::giftbadge_action(host, task->creator, creator_badge_id, std::string("Orginizer badge"), true, true, report->goal_id, report->task_id);					
			
			// };

		}


		// unicore::check_and_gift_netted_badge(report->username, host, task->id);

    // accounts.modify(acc, host, [&](auto &a){
    //   a.approved_reports = acc -> approved_reports + 1;
    //   // a.completed_tasks = task->remain.amount == 0 ? acc -> completed_tasks + 1 : acc -> completed_tasks;
    // });
		
	}

	/**
	 * @brief      Публичный метод отклонения отчета
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::disapprover(eosio::name host, uint64_t report_id, eosio::string comment){
		require_auth(host);
		account_index accounts(_me, _me.value);
		
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		reports_index reports(_me, host.value);
		auto report = reports.find(report_id);

		eosio::check(report->approved == false, "Report is already approved and cannot be disapproved now.");

		reports.modify(report, host, [&](auto &r){
			r.need_check = false;
			r.comment = comment;
		});

	};



/**
	 * @brief      Публичный метод вывода баланса отчёта
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::withdrawrepo(eosio::name username, eosio::name host, uint64_t report_id) {
		require_auth(username);
		account_index accounts(_me, _me.value);
		
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		reports_index reports(_me, host.value);
		auto report = reports.find(report_id);
		
		eosio::check(report->username == username, "wrong username");

		eosio::check(report->balance.amount > 0 || report -> power_balance > 0, "Report does not have a balance for withdraw");
		
		goals_index goals(_me, host.value);
		auto goal = goals.find(report->goal_id);

		goals.modify(goal, username, [&](auto &g) {
			g.withdrawed += report -> balance;
			eosio::check(goal->remain_asset_on_distribution >= report->balance, "System error on distribution");
			g.remain_asset_on_distribution -= report->balance;
			g.remain_power_on_distribution -= report->power_balance;
		});

		uint64_t task_freeze_seconds = unicore::getcondition(host, "tafreezesecs");

		if (report->balance.amount > 0) {
			if (task_freeze_seconds == 0) {
					action(
			      permission_level{ _me, "active"_n },
			      acc->root_token_contract, "transfer"_n,
			      std::make_tuple( _me, report->username, report->balance, std::string("")) 
			    ).send();
			} else {
				make_vesting_action(username, host, acc -> root_token_contract, report->balance, task_freeze_seconds, "distwithdraw"_n);
			};
		};

		// if (report -> power_balance > 0)
		// 	action(
	  //       permission_level{ _me, "active"_n },
	  //       _me, "emitpower"_n,
	  //       std::make_tuple( host , username, report -> power_balance, false) 
	  //   ).send();


		reports.modify(report, username, [&](auto &r){
			r.withdrawed = report->balance;
			r.balance = asset(0, report->balance.symbol);
			r.withdrawed_power = report -> power_balance;
			r.power_balance = 0;
		});
	
	};




/**
	 * @brief      Публичный метод обновления баланса отчёта при распределении баланса цели
	 * 
	 * @param[in]  op    The operation
	 */
	[[eosio::action]] void unicore::distrepo(eosio::name username, eosio::name host, uint64_t report_id) {
		require_auth(username);
		account_index accounts(_me, _me.value);
		
		auto acc = accounts.find(host.value);
		eosio::check(acc != accounts.end(), "Host is not found");

		reports_index reports(_me, host.value);
		auto report = reports.find(report_id);

		goals_index goals(_me, host.value);
		auto goal = goals.find(report->goal_id);
		
		// eosio::check(report->distributed == false, "Distribution already happen to a report");
		eosio::check(goal->finish_at.sec_since_epoch() < eosio::current_time_point().sec_since_epoch(), "Goal is still under distribution");
		// eosio::check(report -> username == username, "Only funds from your report you can withdraw");

		if ((goal->status == "reported"_n || goal->status == "completed"_n) && report->distributed == false) {

			//TODO!
			
			auto liquid_shares = acc->total_loyality > 0 ? acc -> total_loyality : 1;

			uint64_t votes = goal -> positive_votes > goal -> filled_votes ? goal -> positive_votes : goal -> filled_votes; 
			
			int64_t votes_percent = votes * 100 * ONE_PERCENT / liquid_shares;
	    // print("votes_percent: ", votes_percent);

	    // print("acc -> consensus_percent: ", acc -> consensus_percent);
	    // print("goal -> positive_votes: ", goal -> positive_votes);
	    // print("liquid_shares -> :", liquid_shares);

	    eosio::check(votes_percent >= acc -> consensus_percent, "Goal is not validated!");  
			
			double part = (double)report->positive_votes / (double)goal->second_circuit_votes * (double)goal->total_asset_on_distribution.amount;
			eosio::asset part_asset = asset((uint64_t)part, report->balance.symbol);

			uint64_t upower = uint64_t((double)report->positive_votes / (double)goal->second_circuit_votes * (double)goal->total_power_on_distribution);
		
			reports.modify(report, username, [&](auto &r) {
				r.balance += part_asset;
				r.power_balance += upower;
				r.distributed = true;
			});	

		}	
	};

	
