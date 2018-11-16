namespace eosio{
struct core {


    void next_goals(account_name host){
        
        //FIND all goals with activated status sorted by votes 
        //and fill priority enter list until half size of pool will full filled.

        goals_index goals(_self, host);
        
        auto idx_bp = goals.template get_index<N(in_protocol)>();
        auto i_bp = idx_bp.lower_bound(1);
        std::vector<uint64_t> list_for_disable;

        while(i_bp != idx_bp.end()) {
            list_for_disable.push_back(i_bp->id);
            i_bp++;
        };
        
        std::vector<account_name> zero_voters;

        for(auto id : list_for_disable){
            auto goal_for_modify = goals.find(id);
            goals.modify(goal_for_modify, _self, [&](auto &g){
                g.in_protocol = false;
                g.rotation_num += 1;
                g.total_votes = 0;
                g.voters = zero_voters;
            }); 
        };
        
        auto idx_bv = goals.template get_index<N(total_votes)>();
        auto i_bv = idx_bv.rbegin();
        bool filled = false;
        
      
        account_index accounts(_self, _self);
        auto acc = accounts.find(host);
        account_name active_host = acc->get_active_host();

        spiral_index spiral(_self, active_host);
        auto sp = spiral.find(0);
        auto half_of_pool = sp -> size_of_pool / 2;
        uint64_t lepts_filled = 0;
        std::vector<uint64_t> list_for_enable;
        std::vector<uint64_t> list_for_diactivate;

        while(i_bv != idx_bv.rend() && filled == false) {
            if (i_bv->activated == true && i_bv -> validated == true){
                if (i_bv -> available >= i_bv->activation_amount){
                
                    lepts_filled += i_bv -> lepts_for_each_pool;
                    if (lepts_filled <= half_of_pool){
                        list_for_enable.emplace_back(i_bv->id);
                    };
                
                    filled = lepts_filled >= half_of_pool;
                } else {
                    list_for_diactivate.emplace_back(i_bv->id);
                }
            };
            i_bv++;
        };

        for (auto id : list_for_diactivate){
            auto goal_for_diactivate = goals.find(id);

            goals.modify(goal_for_diactivate, _self, [&](auto &g){
                    g.activated = false;
            });
        }

        for (auto id : list_for_enable){
            auto goal_for_modify = goals.find(id);
                
                goals.modify(goal_for_modify, _self, [&](auto &g){
                    g.in_protocol = true;
                    g.available = goal_for_modify->available - goal_for_modify->activation_amount;
                });
                
                gpriorenter op;
            
                op.lepts_for_each_pool = goal_for_modify->lepts_for_each_pool * LEPTS_PRECISION;
                op.username = goal_for_modify->username;
                op.host = host;
                op.goal_id = goal_for_modify -> id;
                    
                priority_goal_enter(op);
                

        } 
                
    }


 void priority_goal_enter(const gpriorenter &op){
        auto username = op.username;
        auto host = op.host;
        auto lepts = op.lepts_for_each_pool;
        auto goal_id = op.goal_id;

        refresh_state(host);
        
        account_index accounts(_self, _self);
        auto acc = accounts.find(host);
        auto main_host = acc->get_active_host();
         
        pool_index pools(_self, host);
        balance_index balances(_self, username);

        cycle_index cycles(_self, host);
         
        rate_index rates(_self, main_host);

        auto root_symbol = acc->get_root_symbol();
         
        auto first_pool_rate = rates.find(0);
        auto second_pool_rate = rates.find(1);

        auto cycle = cycles.find(acc-> current_cycle_num - 2);
         
        eosio_assert(acc->current_pool_num < 2, "Priority Enter is available only for first 2 rounds in cycle");
        eosio_assert(acc->priority_flag == true, "Priority Enter is available only when priority_flag is enabled");
        
        auto first_pool = pools.find(cycle->finish_at_global_pool_id + 1);
        auto second_pool = pools.find(cycle->finish_at_global_pool_id + 2);
         
        eosio::asset first_pool_amount = asset(lepts * first_pool_rate->buy_rate / LEPTS_PRECISION, root_symbol);
        eosio::asset second_pool_amount = asset(lepts * second_pool_rate->buy_rate / LEPTS_PRECISION, root_symbol);
        eosio::asset amount_for_back = first_pool_amount + second_pool_amount;
       
        uint64_t bal_id1 = fill_pool(username, host, lepts, first_pool_amount, acc-> current_pool_id, true, goal_id);
        uint64_t bal_id2 = fill_pool(username, host, lepts, second_pool_amount, acc-> current_pool_id + 1, true, goal_id);
        
        add_balance_id_to_goal(host, goal_id, bal_id1);
        add_balance_id_to_goal(host, goal_id, bal_id2);
    }


void adjust_goal_balance(account_name host, uint64_t goal_id, eosio::asset amount){
    goals_index goals (_self, host);
    //CHECK FOR SYMBOL
    
    //Check for amount < nominal - diactivate

    bool disable = false;
    bool is_complete = false;
    auto goal = goals.find(goal_id);
    
    eosio::asset available = goal -> available + amount;

    if (available < goal->activation_amount)    
        disable = true;

    if (available >= goal->target) {
        disable = true;
        is_complete = true;
    }


    goals.modify(goal, _self, [&](auto &g){
        g.completed = is_complete;
        g.activated = !disable; 
        g.available = available;
    });

    //Здесь мы принимаем платежи от Протокола и вносим их на баланс цели. 
};


void add_balance_id_to_goal(account_name host, uint64_t goal_id, uint64_t balance_id){
    goals_index goals(_self, host);
    auto goal = goals.find(goal_id);
    std::vector <uint64_t> balance_ids = goal->balance_ids;
    balance_ids.push_back(balance_id);

    goals.modify(goal, _self, [&](auto &g){
        g.balance_ids = balance_ids;
    });

}

void delete_balance_id_from_goal(account_name host, uint64_t goal_id, uint64_t balance_id){
    goals_index goals(_self, host);
    auto goal = goals.find(goal_id);
    std::vector <uint64_t> balance_ids = goal->balance_ids;
    auto itr = std::find(balance_ids.begin(), balance_ids.end(), balance_id);
    
    if (itr != balance_ids.end()){
        balance_ids.erase(itr);
        goals.modify(goal, _self, [&](auto &g){
            g.balance_ids = balance_ids;
        });
    };

}

void priority_enter(const priorenter &op){
     auto username = op.username;
     auto host = op.host;
     auto balance_id = op.balance_id;
     
     refresh_state(host); 

     pool_index pools(_self, host);
     balance_index balances(_self, username);
     cycle_index cycles(_self, host);
     rate_index rates(_self, host);
     account_index accounts(_self, _self);

     auto acc = accounts.find(host);
     auto main_host = acc->get_active_host();
     auto root_symbol = acc->get_root_symbol();


     auto first_pool_rate = rates.find(0);
     auto second_pool_rate = rates.find(1);

     auto cycle = cycles.find(acc-> current_cycle_num - 2);
     
     eosio_assert(acc->current_pool_num < 2, "Priority Enter is available only for first 2 rounds in cycle");
     eosio_assert(acc->priority_flag == true, "Priority Enter is available only when priority_flag is enabled");
    
     auto bal = balances.find(balance_id);
     eosio_assert(bal != balances.end(), "Balance not exist");
     eosio_assert(bal->cycle_num == acc->current_cycle_num - 1, "Only participants from previous cycle can priority enter");
     eosio_assert(bal->last_recalculated_win_pool_id == cycle->finish_at_global_pool_id, "Impossible priority enter with not refreshed balance");
     eosio_assert(bal->win == 0, "Only lose balances can enter by priority");
     eosio_assert(bal->withdrawed == 0, "Balances withdrawed and cant enter on priority");
     eosio_assert(bal->available != bal->purchase_amount, "Nominal dont have priority");

     auto first_pool = pools.find(cycle->finish_at_global_pool_id + 1);
     auto second_pool = pools.find(cycle->finish_at_global_pool_id + 2);
     
     eosio_assert(first_pool->remain_lepts == second_pool->remain_lepts, "Oops, something going wrong. Priority enter is closed.");
     
     uint64_t first_pool_lepts = bal->next_lept_for_sale / 2;
     uint64_t second_pool_lepts = first_pool_lepts;
     
     eosio::asset first_pool_amount = asset(first_pool_lepts * first_pool_rate->buy_rate / LEPTS_PRECISION, root_symbol);
     eosio::asset second_pool_amount = asset(second_pool_lepts * second_pool_rate->buy_rate / LEPTS_PRECISION, root_symbol);
     eosio::asset total_enter = first_pool_amount + second_pool_amount;
     

     //compare available amount with amount by next_lept_for_sale. 
     //If calculated amount by next_lept_for_sale is more then available amount,
     //then recalculate enter amount by available amount through lept rate for first pools
     //(user will get less lepts, but total balance will be in safety);
     //it will happen only on small distance from enter to exit pool

     if (bal->available >= total_enter){
        eosio::asset amount_for_back = asset(bal->available - total_enter);
     if (bal->is_goal == false){
             action(
                    permission_level{ _self, N(active) },
                    acc->root_token_contract, N(transfer),
                    std::make_tuple( _self, username, amount_for_back, std::string("Sediment amount")) 
            ).send();
        } else {
            adjust_goal_balance(bal->host, bal->goal_id, amount_for_back);
        };
    } else {
        //У пользователя есть доступная сумма баланса в токенах, которая меньше чем та, с которой участник 
        //может зайти в приоритете в лептах. Надо найти минимальную сумму в лептах, которая удовлетворяет
        //условиям баланса. Для этого берем курс покупки второго раунда, и высчитаем количество лепт по нему. 
        //Затем вычисляем суммы для первого и второго раунда, а разницу - возвращаем. 
        second_pool_lepts = (bal->available).amount / 2 * LEPTS_PRECISION / second_pool_rate->buy_rate;
    
        first_pool_lepts = second_pool_lepts;
        first_pool_amount = asset(first_pool_lepts * first_pool_rate->buy_rate / LEPTS_PRECISION, root_symbol);
    
        second_pool_amount = asset(second_pool_lepts * second_pool_rate->sell_rate / LEPTS_PRECISION, root_symbol);
        eosio::asset amount_for_back = bal->available - (first_pool_amount + second_pool_amount);
        
        action(
            permission_level{ _self, N(active) },
            acc->root_token_contract, N(transfer),
            std::make_tuple( _self, username, amount_for_back, std::string("Sediment amount")) 
        ).send();
    
    }
     
    
     uint64_t bal_id1 = fill_pool(username, host, first_pool_lepts, first_pool_amount, acc-> current_pool_id, bal->is_goal, bal->goal_id);
     uint64_t bal_id2 = fill_pool(username, host, second_pool_lepts, second_pool_amount, acc-> current_pool_id + 1, bal->is_goal, bal->goal_id);
     
     if (bal->is_goal == true){
            add_balance_id_to_goal(host, bal->goal_id, bal_id1);
            add_balance_id_to_goal(host, bal->goal_id, bal_id2);
            delete_balance_id_from_goal(host, bal->goal_id, bal->id);                  
     };

        auto last_pool = pools.find(cycle->finish_at_global_pool_id);

        pools.modify(last_pool, username, [&](auto &p){
             p.total_loss_withdraw = last_pool -> total_loss_withdraw + bal->available;
        });

        std::vector<eosio::asset> forecasts;
        balances.modify(bal, username, [&](auto &b){
            b.sold_amount = bal -> available;
            b.date_of_sale = eosio::time_point_sec(now());
            b.lept_for_sale = 0; 
            b.withdrawed = true;
            b.available = asset(0, root_symbol);
            b.forecasts = forecasts;
        });

}


void adjust_clan_income (account_name parent_host, account_name last_active_host){
    account_index accounts(_self, _self);
    auto acc = accounts.find(parent_host);   
    sincome_index sincomes(_self, parent_host);

    pool_index pools(_self, parent_host);
    spiral_index spiral(_self, last_active_host);
    rate_index rates(_self, last_active_host);

    auto sp = spiral.find(0);
    auto pool = pools.find(acc -> current_pool_id);
    
    //Добавляем бизнес-доход предыдущего цикла в таблицу без учета последнего пула.
    if ((pool-> pool_num > 2) && (pool->pool_num < sp->pool_limit - 1)){
        auto prev_rate = rates.find(pool->pool_num - 1);
        auto sinc = sincomes.find(acc->current_cycle_num - 1);

        sincomes.modify(sinc, _self, [&](auto &s){
            s.pool_num = pool -> pool_num;
            s.available = prev_rate -> system_income;
        });
    }    

}

void improve_params_of_new_cycle (account_name host, account_name main_host){
        account_index accounts(_self, _self);
        auto acc = accounts.find(host);
        auto root_symbol = acc->get_root_symbol();
        
        sincome_index sincomes(_self, host);
        cycle_index cycles(_self, host);
        

            cycles.emplace(_self, [&](auto &c){
                c.id = cycles.available_primary_key();
                c.active_host = main_host;
                c.start_at_global_pool_id = acc->current_pool_id + 1; 
                c.finish_at_global_pool_id = acc->current_pool_id + 2;    
            });

            accounts.modify(acc, _self, [&](auto &dp){
                dp.current_pool_id  = acc->current_pool_id + 1;
                dp.cycle_start_at_id = dp.current_pool_id;
                dp.current_cycle_num = acc->current_cycle_num + 1;
                dp.current_pool_num  = 1;
                dp.priority_flag = true;       
            });
            

            sincomes.emplace(_self, [&](auto &sinc){
                sinc.active_host = main_host;
                sinc.id = sincomes.available_primary_key();
                sinc.pool_num = 0;
                sinc.available = asset(0, root_symbol);
            });
}
 

    void emplace_first_pools(account_name parent_host, account_name main_host, eosio::symbol_name root_symbol){
        
        spiral_index spiral(_self, main_host);
        auto sp = spiral.find(0);
        eosio_assert(sp != spiral.end(), "Protocol is not found. Set parameters at first");
        sincome_index sincomes(_self, parent_host);
        
        account_index accounts(_self, _self);
        auto acc = accounts.find(parent_host);
        cycle_index cycles(_self, parent_host);
        pool_index pools(_self, parent_host);

        rate_index rates(_self, main_host);
        
        auto rate = rates.find(0);
        auto next_rate = rates.find(1);
        auto available_id = pools.available_primary_key();
        pools.emplace(_self, [&](auto &p){
            p.id = available_id;
            p.active_host = main_host;
            p.total_lepts = sp->size_of_pool * LEPTS_PRECISION;
            p.creserved_lepts = 0;
            p.remain_lepts = p.total_lepts;
            p.lept_cost = asset(rate->buy_rate, root_symbol);
            p.total_win_withdraw = asset(0, root_symbol);
            p.total_loss_withdraw = asset(0, root_symbol);
            p.pool_num = 1;
            p.cycle_num = acc->current_cycle_num;
            p.pool_started_at = eosio::time_point_sec(now());
            p.priority_until = acc->current_cycle_num == 1 ? eosio::time_point_sec(0) : eosio::time_point_sec(now()+ sp->priority_seconds);;
            p.pool_expired_at = acc->current_cycle_num == 1 ? eosio::time_point_sec (now() + sp->pool_timeout) : eosio::time_point_sec (now() + sp->pool_timeout + sp->priority_seconds);
            p.color = "black";
        });

          pools.emplace(_self, [&](auto &p){
                p.id = available_id + 1;
                p.active_host = main_host;
                p.total_lepts = sp->size_of_pool * LEPTS_PRECISION;
                p.creserved_lepts = 0;
                p.remain_lepts = p.total_lepts;
                p.lept_cost = asset(next_rate->buy_rate, root_symbol);
                p.color = p.id % 2 == 0 ? "black" : "white"; 
                p.cycle_num = acc->current_cycle_num;
                p.pool_num = 2;
                p.pool_started_at = eosio::time_point_sec(now());
                p.priority_until = acc->current_cycle_num == 1 ? eosio::time_point_sec(0) : eosio::time_point_sec(now()+ sp->priority_seconds);;
                p.pool_expired_at = acc->current_cycle_num == 1 ? eosio::time_point_sec (now() + sp->pool_timeout) : eosio::time_point_sec (now() + sp->pool_timeout + sp->priority_seconds);
                p.total_win_withdraw = asset(0, root_symbol);
                p.total_loss_withdraw = asset(0, root_symbol);
            }); 
    }


void start_new_cycle ( account_name host ) {
        account_index accounts(_self, _self);
        sincome_index sincomes(_self, host);
        cycle_index cycles(_self, host);
                
        auto acc = accounts.find(host);
        account_name main_host = acc->get_active_host();
        account_name last_active_host = acc->active_host;
                
        auto root_symbol = acc->get_root_symbol();
            
        if (acc->need_switch) {

            main_host = (acc->childrens).back();
            
            accounts.modify(acc, _self, [&](auto &a){
                a.need_switch = false;
                a.active_host = main_host;
            });

            adjust_clan_income(host, last_active_host);
            improve_params_of_new_cycle(host, main_host);
           emplace_first_pools(host, main_host, root_symbol);

        } else {
            rate_index rates(_self, main_host);
            spiral_index spiral(_self, main_host);
            
            auto cycle = cycles.find(acc-> current_cycle_num - 1);
            auto rate = rates.find(0);
            auto next_rate = rates.find(1);
            auto sp = spiral.find(0);
            
            adjust_clan_income(host, last_active_host);
            
            pool_index pools(_self, host);
            uint64_t available_id = pools.available_primary_key();
            improve_params_of_new_cycle(host, main_host);
            
            emplace_first_pools(host, main_host, root_symbol);

        }

            next_goals(host);
            
            refresh_state(host);  
            
    };

    void next_pool( account_name host){
        account_index accounts(_self, _self);

        auto acc = accounts.find(host);
        auto main_host = acc->get_active_host();
        
        sincome_index sincomes(_self, host);
        cycle_index cycles(_self, host);
        pool_index pools(_self, host);

        rate_index rates(_self, main_host);
        spiral_index spiral(_self, main_host);
        

        auto root_symbol = acc->get_root_symbol();
        auto pool = pools.find(acc -> current_pool_id);
        auto cycle = cycles.find(acc -> current_cycle_num - 1);
        
        auto sp = spiral.find(0);
        
        uint128_t dreserved_lepts = 0;
        uint64_t reserved_lepts = 0;

        
        //Если первые два пула не до конца закрыты, это значит, 
        //не все реинвестировали, и пул добавлять не нужно. 
            
        if (acc -> current_pool_num > 1) {
            auto rate = rates.find(acc-> current_pool_num);
            //Если это обычный пул, то добавляем по 1\

             cycles.modify(cycle, _self, [&](auto &c ){
                c.finish_at_global_pool_id = cycle -> finish_at_global_pool_id + 1;
            });

            accounts.modify(acc, _self, [&](auto &dp){
               dp.current_pool_num = pool -> pool_num + 1;
               dp.current_pool_id  = pool -> id + 1;
            });
            
            auto prev_prev_pool = pools.find(acc -> current_pool_id - 2);
            
            dreserved_lepts = (prev_prev_pool -> total_lepts - prev_prev_pool -> remain_lepts ) / LEPTS_PRECISION  * rate -> sell_rate / rate -> buy_rate  \
                  * LEPTS_PRECISION;

            reserved_lepts = uint64_t(dreserved_lepts);

            if ((pool-> pool_num > 1) && (pool->pool_num < sp->pool_limit )){
                auto sinc = sincomes.find(acc->current_cycle_num - 1);
            
                sincomes.modify(sinc, _self, [&](auto &s){
                    s.pool_num = pool -> pool_num;
                    s.available = rate -> system_income;
                });
            }
                
            pools.emplace(_self, [&](auto &p){
                p.id = pools.available_primary_key();
                p.active_host = main_host;
                p.total_lepts = sp->size_of_pool * LEPTS_PRECISION;
                p.creserved_lepts = prev_prev_pool -> creserved_lepts;
                p.remain_lepts = p.total_lepts - reserved_lepts;
                p.lept_cost = asset(rate->buy_rate, root_symbol);
                p.color = p.id % 2 == 0 ? "black" : "white"; 
                p.cycle_num = pool -> cycle_num;
                p.pool_num = pool -> pool_num + 1;
                p.pool_started_at = eosio::time_point_sec(now());
                p.priority_until = eosio::time_point_sec(0);
                p.pool_expired_at = eosio::time_point_sec (now() + sp->pool_timeout);
                p.total_win_withdraw = asset(0, root_symbol);
                p.total_loss_withdraw = asset(0, root_symbol);
            });

             
        
        } else {
                auto rate = rates.find(acc-> current_pool_num);
                
                accounts.modify(acc, _self, [&](auto &a){
                   a.current_pool_num = pool -> pool_num + 1;
                   a.current_pool_id  = pool -> id + 1;
                   a.priority_flag = false;     
                });
                
        }

        


        
    };

   

    void start_action (const start &op){
        if (op.host == op.child_host)
            require_auth(op.host);
        else 
            require_auth(op.child_host);

        account_index accounts(_self, _self);
        auto main_host = op.host;
        
        sincome_index sincomes (_self, main_host);
        cycle_index cycles (_self, op.host);

        auto account = accounts.find(main_host);
        auto root_symbol = account -> get_root_symbol();
        eosio_assert(account != accounts.end(), "Account is not upgraded to Host. Please update your account and try again.");
        eosio_assert(account->parameters_setted == true, "Cannot start host without setted parameters");
        
        auto child_hosts = account->childrens;

        if (child_hosts.begin() == child_hosts.end()) {

            eosio_assert(account->activated == false, "Protocol is already active and cannot be changed now.");
            
            accounts.modify(account, _self, [&](auto &a){
                a.activated = true;
            });
                 // auto first_cycle = cycles.find(0);

            cycles.emplace(_self, [&](auto &c){
                c.active_host = main_host;
                c.id = cycles.available_primary_key();
                c.start_at_global_pool_id = 0;
                c.finish_at_global_pool_id = 1;
            });

            sincomes.emplace(_self, [&](auto &sinc){
                sinc.active_host = main_host;
                sinc.id = 0;
                sinc.pool_num = 0;
                sinc.available = asset(0, root_symbol);
            });

            emplace_first_pools(op.host, main_host, root_symbol);
        
        } else {
            
            eosio_assert(account->parameters_setted == true, "Cant start branch without setted parameters");
            main_host = child_hosts.back();
            eosio_assert(account->active_host != main_host, "Protocol is already active and cannot be changed now.");
            eosio_assert(main_host == op.child_host, "Wrong last non-active child host");
            
            accounts.modify(account, _self, [&](auto &a){
                a.need_switch = true;
                a.non_active_child = false;
            });
        };
    }


    void setfee_action(const setfee &op){
        require_auth(_dacomfee);
    
        fee_index fee(_self,_self);

        auto fe = fee.find(0);
        if (fe == fee.end()){
            fee.emplace(_self, [&](auto &f){
                f.createhost = op.createhost;
                f.systemfee = op.systemfee;
            });
        } else {
     
           fee.modify(fe, _dacomfee, [&](auto &f){
                f.createhost = op.createhost;
                f.systemfee = op.systemfee;
            });
        }
    }

    void setparams_action(const setparams &op){
        if (op.host == op.child_host)
            require_auth(op.host);
        else 
            require_auth(op.child_host);

        account_index accounts(_self, _self);
        auto main_host = op.host;
        auto account = accounts.find(main_host);

        auto root_symbol = account -> get_root_symbol();
        auto child_hosts = account->childrens;

        if (account->non_active_child == true) {
            main_host = child_hosts.back();
            eosio_assert(account->active_host != main_host, "Protocol is already active and cannot be changed now.");
            eosio_assert(main_host == op.child_host, "Wrong last non-active child host");
            spiral_index last_params (_self, account->active_host);
            auto lp = last_params.find(0);
            eosio_assert(lp->size_of_pool <= op.size_of_pool, "Size of pool cannot be decreased in child host");
        } else {
            eosio_assert(account->activated == false, "Protocol is already active and cannot be changed now.");
        }

        
        eosio_assert(account != accounts.end(), "Account is not upgraded to Host. Please update your account and try again.");
        eosio_assert(account->payed == true, "Host is not payed and can not start");


        rate_index rates(_self, main_host);
        spiral_index spiral(_self, main_host);
        
        auto size_of_pool = op.size_of_pool;
        auto overlap = op.overlap;
        auto profit_growth = op.profit_growth;
        auto base_rate = op.base_rate;
        auto loss_percent = op.loss_percent;
        auto pool_limit = op.pool_limit;
        auto pool_timeout = op.pool_timeout;
        auto priority_seconds = op.priority_seconds;

        eosio_assert((overlap > 10000) && (overlap < 20000), "Overlap factor must be greater then 10000 (1.0000) and less then 20000 (2.0000)).");
        eosio_assert(profit_growth <= 20000, "Profit growth factor must be greater or equal 0 (0.0000) and less then 20000 (2.0000)).");
        eosio_assert((loss_percent > 0 ) && ( loss_percent <= 10000), "Loss Percent must be greater then 0 (0%) and less or equal 10000 (100%)");
        eosio_assert((base_rate >= 10) && (base_rate < 1000000000), "Base Rate must be greater or equal 10 and less then 1e9");
        eosio_assert((size_of_pool >= 10) && (size_of_pool < 1000000000), "Size of Pool must be greater or equal 10 and less then 1e9");
        eosio_assert((pool_limit >= 3) && (pool_limit < 1000), "Pool Count must be greater or equal 4 and less or equal 1000");
        eosio_assert((pool_timeout >= 60) && (pool_timeout < 7884000),"Pool Timeout must be greater or equal then 60 sec and less then 7884000 sec");
        
        //s.priority_seconds 
        auto sp = spiral.find(0);
        auto rate = rates.find(0);
        
        if (((sp != spiral.end()) && rate != rates.end()) && ((account -> activated == false) || (account->active_host != main_host))){
            spiral.erase(sp);
            auto it = rates.begin();
            while (it != rates.end()) {
                it = rates.erase(it);
            };
        };

        spiral.emplace(_self, [&](auto &s){
            s.overlap = overlap;
            s.size_of_pool = size_of_pool;
            s.profit_growth = profit_growth;
            s.base_rate = base_rate;
            s.loss_percent = loss_percent;
            s.pool_limit = pool_limit;
            s.pool_timeout = pool_timeout;
            s.priority_seconds = priority_seconds;
        });
        
        uint64_t buy_rate[pool_limit];
        uint64_t sell_rate[pool_limit];
        uint64_t delta[pool_limit];
        uint64_t client_income[pool_limit];
        uint64_t pool_cost[pool_limit];
        uint64_t total_in_box[pool_limit];
        uint64_t payment_to_wins[pool_limit];
        uint64_t payment_to_loss[pool_limit];
        uint64_t system_income[pool_limit];
        uint64_t live_balance_for_sale[pool_limit];
        
        for (auto i=0; i < pool_limit - 1; i++ ){
             
             if (i == 0){
                buy_rate[i] = base_rate;
                sell_rate[i] = base_rate;
                delta[i] = 0;
                delta[i+1] = 0;
                client_income[i] = 0;
                pool_cost[i] = size_of_pool * buy_rate[i];
                total_in_box[i] = pool_cost[i];
                payment_to_wins[i] = 0;
                payment_to_loss[i] = 0;
                system_income[i] = 0;
                live_balance_for_sale[i] = total_in_box[i];

            } else if (i > 0){ 
                sell_rate[i + 1] = buy_rate[i-1] * overlap / PERCENT_PRECISION;
                client_income[i+1] = sell_rate[i+1] - buy_rate[i-1];
                delta[i+1] = i > 1 ? client_income[i+1] * profit_growth / PERCENT_PRECISION + delta[i] : client_income[i+1];
                buy_rate[i] = sell_rate[i+1] + delta[i+1];
                pool_cost[i] = size_of_pool * buy_rate[i];
                payment_to_wins[i] = i > 1 ? size_of_pool * sell_rate[i] : 0;
                live_balance_for_sale[i] = pool_cost[i] - payment_to_wins[i];
                
                if (i == 1){
                    sell_rate[i] = buy_rate[i];
                    payment_to_loss[i] = 0;
                
                } else if (i == 2){
                    payment_to_loss[i] = live_balance_for_sale[i-1] * (PERCENT_PRECISION - loss_percent) / PERCENT_PRECISION;
                } else if ( i == 3 ){
                    payment_to_loss[i] = live_balance_for_sale[i-3] * (PERCENT_PRECISION - loss_percent) / PERCENT_PRECISION + live_balance_for_sale[i-1] * (PERCENT_PRECISION - loss_percent) / PERCENT_PRECISION;
                } else if ( i > 3 ){
                    payment_to_loss[i] = live_balance_for_sale[i-1] * (PERCENT_PRECISION - loss_percent) / PERCENT_PRECISION + payment_to_loss[i - 2];
                }
                
                system_income[i] = i > 1 ? total_in_box[i-1] - payment_to_wins[i] - payment_to_loss[i] : 0; 
                total_in_box[i] = i > 1 ? total_in_box[i-1] + live_balance_for_sale[i] : total_in_box[i-1] + pool_cost[i];

                bool positive = total_in_box[i-1] - payment_to_wins[i] <= payment_to_loss[i] ? false : true;
                
                eosio_assert(positive, "The financial model of Protocol is Negative. Try with a different parameters");

                eosio_assert(total_in_box[i-1] < _MAX_SUPPLY, "Tokens in the Box are exceeds the max supply. Try to decrease Protocol parameters.");
                
                if (i > 2)
                    eosio_assert((client_income[i-1] > 0), "Overlap rate is too small for Protocol operation. Try to increase Overlap parameter, Size of Pool or Base Rate");      
            } 
        };


        for (auto i = 0; i < pool_limit - 1; i++){
            rates.emplace(_self, [&](auto &r){
                    r.pool_id = i;
                    r.buy_rate = buy_rate[i];
                    r.sell_rate = sell_rate[i];
                    r.client_income = asset(client_income[i], root_symbol);
                    r.delta = asset(delta[i], root_symbol);
                    r.total_lepts = size_of_pool;
                    r.pool_cost = asset(pool_cost[i], root_symbol);
                    r.payment_to_wins = asset(payment_to_wins[i], root_symbol);
                    r.payment_to_loss = asset(payment_to_loss[i], root_symbol);
                    r.total_in_box = asset(total_in_box[i], root_symbol);
                    r.system_income = asset(system_income[i], root_symbol);
                    r.live_balance_for_sale = asset(live_balance_for_sale[i], root_symbol);
                });
        }

        accounts.modify(account, _self, [&](auto &a){
            a.parameters_setted = true;
        });
    }


    
    void deposit ( account_name username, account_name host, eosio::asset amount, account_name code ){
        require_auth(username);
        eosio_assert( amount.is_valid(), "Rejected. Invalid quantity" );

        refresh_state(host);
        account_index accounts(_self, _self);
        auto acc = accounts.find(host);
        eosio_assert(acc->root_token_contract == code, "Wrong token contract for this host");

        account_name main_host = acc->get_active_host();
        
        if (acc->is_whitelisted){
            auto user_is_whitelisted = acc->is_account_in_whitelist(username);
            eosio_assert(user_is_whitelisted, "Username not founded in the host whitelist"); 
        }

        rate_index rates(_self, main_host);
        pool_index pools(_self, host);
        
        auto root_symbol = acc->get_root_symbol();

        
        eosio_assert ( amount.symbol == root_symbol, "Rejected. Invalid symbol for this contract.");
        eosio_assert(acc != accounts.end(), "Rejected. Host is not founded.");
        eosio_assert(acc -> activated == true, "Rejected. Protocol is not active");

        auto pool = pools.find( acc -> current_pool_id );
        //eosio_assert(pool -> remain_lepts <= pool->total_lepts, "Prevented Deposit. System error");

        eosio_assert( acc-> priority_flag == false, "This is a Priority Time");

        eosio_assert ( pool -> pool_expired_at > eosio::time_point_sec(now()), "Rejected. Pool is Expired. Need manual refresh.");
        
        auto rate = rates.find( pool-> pool_num - 1 );
        eosio_assert(amount.amount % rate -> buy_rate == 0, "You can purchase only whole pieces of LEPT. Specify the amount of the multiple purchase rate.");

        uint64_t lepts = LEPTS_PRECISION * amount.amount / rate -> buy_rate;
        eosio_assert(pool -> remain_lepts >= lepts, "Not enought LEPT in target pool");
        
        fill_pool(username, host, lepts, amount, acc -> current_pool_id);
    
        refresh_state(host);
    };



    void refresh_state (account_name host){
 
        account_index accounts(_self, _self);
        auto acc = accounts.find(host);
        account_name main_host = acc->get_active_host();

        pool_index pools(_self, host);
        spiral_index spiral(_self, main_host);

        auto sp = spiral.find(0);
        auto pool = pools.find(acc -> current_pool_id);
        if (acc -> priority_flag == true){
            if (pool->pool_started_at + sp->priority_seconds < eosio::time_point_sec(now()) ||
                (pool -> remain_lepts < LEPTS_PRECISION)){

                accounts.modify(acc, _self, [&](auto &dp){
                    dp.priority_flag = false;
                });
        
                pools.modify(pool, _self, [&](auto &p){
                    p.remain_lepts = pool->remain_lepts / LEPTS_PRECISION * LEPTS_PRECISION;
                });

                
                auto next_pool = pools.find(acc->current_pool_id + 1);
                pools.modify(next_pool, _self, [&](auto &p ){
                    p.remain_lepts = next_pool->remain_lepts / LEPTS_PRECISION * LEPTS_PRECISION; 
                });

                refresh_state(host);

            }
        } else {
            //Если пул истек, или доступных пулов больше нет, или оставшихся лепт больше нет, то новый цикл
            if ((pool -> pool_expired_at < eosio::time_point_sec(now()) || \
                ((pool -> pool_num + 1 == sp->pool_limit) && (pool -> remain_lepts == 0)))){
                start_new_cycle(host);
          
            } else if ((pool -> remain_lepts < LEPTS_PRECISION)){
            //Если просто нет лепт - новый пул. 
            //На случай, если приоритетные пулы полностью заполнены с остатком менее 1 LEPT, повторный рефреш. 
                next_pool(host);
                refresh_state(host);

            } else if (acc->current_pool_num < 3) {
                //Отдельно округляем остатки в случае, если приоритетным входом или целями 
                //воспользовались только частично
                pools.modify(pool, _self, [&](auto &p ){
                    p.remain_lepts = pool->remain_lepts / LEPTS_PRECISION * LEPTS_PRECISION; 
                });
            }
        }
        
    };


    uint64_t fill_pool(account_name username, account_name host, uint64_t lepts, eosio::asset amount, uint64_t filled_pool_id, bool is_goal = false, uint64_t goal_id = 0){
        std::vector<eosio::asset> forecasts;
        account_index accounts(_self, _self);
        
        auto acc = accounts.find(host);
        auto main_host = acc -> get_active_host();

        cycle_index cycles(_self, host);
        pool_index pools(_self, host);
        rate_index rates(_self, main_host);
        
        balance_index balance(_self, username);
        
        auto root_symbol = acc->get_root_symbol();
        auto pool = pools.find(filled_pool_id);
        eosio_assert(pool != pools.end(), "Cant FIND");
        auto rate = rates.find(acc->current_pool_num + 1);
        uint64_t next_lept_for_sale;
        uint64_t remain_lepts;

        if (rate == rates.end())
            next_lept_for_sale = lepts;
        else 
            next_lept_for_sale = lepts * rate->sell_rate / rate->buy_rate;
        
        if (lepts > pool -> remain_lepts){
       
            uint64_t floor_remain_lepts = LEPTS_PRECISION * pool->remain_lepts / LEPTS_PRECISION + LEPTS_PRECISION;
            eosio_assert(lepts <= floor_remain_lepts, "Oops, something wrong.");
            remain_lepts = floor_remain_lepts - lepts;
        } else {
            remain_lepts = pool -> remain_lepts - lepts;
        }
        
        forecasts = calculate_forecast(username, host, lepts, pool -> pool_num - 1);

        pools.modify(pool, _self, [&](auto &p){
            p.remain_lepts = remain_lepts;
//          p.remain_lepts = LEPTS_PRECISION * remain_lepts / LEPTS_PRECISION;
        });

        auto b_id = balance.available_primary_key();
        balance.emplace(_self, [&](auto &b){
            b.id = b_id;
            b.cycle_num = pool->cycle_num;
            b.pool_num = pool->pool_num;
            b.host = host;
            b.children_host = main_host;
            b.global_pool_id = filled_pool_id; 
            b.pool_color = pool -> color;
            b.lept_for_sale = lepts;
            b.next_lept_for_sale = next_lept_for_sale;
            b.purchase_amount = amount;
            b.available = amount;
            b.date_of_purchase = eosio::time_point_sec(now());
            b.last_recalculated_win_pool_id = pool -> id;
            b.forecasts = forecasts;
            b.sold_amount = asset(0, root_symbol);
            b.is_goal = is_goal;
            b.goal_id = goal_id;
        });

        return b_id;
    };




    void refresh_balance_action (const refreshbal &op){
        require_auth(op.username);
        account_name username = op.username;
        uint64_t balance_id = op.balance_id;

        std::vector<eosio::asset> forecasts;
        account_index accounts(_self, _self);
        balance_index balance (_self, username);
        auto bal = balance.find(balance_id);
        eosio_assert(bal != balance.end(), "Balance is not exist or already withdrawed");
        
        auto child_host = bal -> get_active_host();
        auto parent_host = bal -> host;
        auto acc = accounts.find(parent_host);

        auto root_symbol = acc->get_root_symbol();
        
        refresh_state(parent_host);
        
        cycle_index cycles(_self, parent_host);
        rate_index rates(_self, child_host);
        pool_index pools(_self, parent_host);
        
        
        auto pool_start = pools.find(bal -> global_pool_id);
        auto cycle = cycles.find(pool_start -> cycle_num - 1);
        auto next_cycle = cycles.find(pool_start -> cycle_num);
        auto pools_in_cycle = cycle -> finish_at_global_pool_id - cycle -> start_at_global_pool_id + 1;
        
        auto last_pool = pools.find(cycle -> finish_at_global_pool_id );
        auto has_new_cycle = false;

        if (next_cycle != cycles.end())
            has_new_cycle = true;

        uint64_t ceiling;
        if (( op.partrefresh == true )&&(bal -> last_recalculated_win_pool_id + 10 < cycle -> finish_at_global_pool_id)){
            ceiling = bal -> last_recalculated_win_pool_id + 10;

        } else {
            ceiling = cycle -> finish_at_global_pool_id;
        }


        if (bal -> last_recalculated_win_pool_id <= cycle -> finish_at_global_pool_id)
            for (auto i = bal -> last_recalculated_win_pool_id + 1;  i <= ceiling; i++){
                auto look_pool = pools.find(i);
                auto purchase_amount = bal-> purchase_amount;
                
                if (((acc -> current_pool_num == pool_start -> pool_num ) && (acc -> current_cycle_num == pool_start -> cycle_num)) || \
                    ((pool_start -> pool_num < 3) && (pools_in_cycle < 3)) || (has_new_cycle && (pool_start->pool_num == last_pool -> pool_num)))

                {
                    balance.modify(bal, username, [&](auto &b){
                          b.last_recalculated_win_pool_id = i;
                          b.available = purchase_amount;
                    });

                } else {
                    
                    eosio::asset available;
                    uint64_t new_reduced_lepts;
                    uint64_t new_lept_for_sale;

                    if (pool_start -> color == look_pool -> color){
                        auto rate = rates.find(look_pool -> pool_num - 1);

                        if (look_pool -> pool_num - pool_start -> pool_num <= 2){
                            
                            new_reduced_lepts = bal -> lept_for_sale * rate -> sell_rate / rate -> buy_rate;
                            new_lept_for_sale = bal->lept_for_sale;
                            
                            if (new_reduced_lepts == 0)
                                new_reduced_lepts = new_lept_for_sale;

                            forecasts = calculate_forecast(username, parent_host, new_reduced_lepts, look_pool -> pool_num - 1);
                            available = asset(bal ->lept_for_sale * rate -> sell_rate / LEPTS_PRECISION, root_symbol);
                        
                        } else {

                            auto prev_win_rate = rates.find(look_pool -> pool_num - 3);
                            new_lept_for_sale = bal -> lept_for_sale * prev_win_rate -> sell_rate / prev_win_rate -> buy_rate;
                            new_reduced_lepts = new_lept_for_sale * rate -> sell_rate / rate -> buy_rate;
                            
                            if (new_reduced_lepts == 0)
                                new_reduced_lepts = new_lept_for_sale;
                            
                            forecasts = calculate_forecast(username, parent_host, new_reduced_lepts, look_pool -> pool_num - 1);
                            available = asset(new_lept_for_sale * rate -> sell_rate / LEPTS_PRECISION, root_symbol);                        
                        
                        }
                       
                        balance.modify(bal, username, [&](auto &b){
                            b.last_recalculated_win_pool_id = i;
                            b.lept_for_sale = new_lept_for_sale;
                            b.next_lept_for_sale = new_reduced_lepts;
                            b.available = available;
                            b.win = true;
                            b.forecasts = forecasts;
                        });

                    } else {

                        spiral_index spiral(_self, parent_host);
                        auto sp = spiral.find(0);
                        balance.modify(bal, username, [&](auto &b){
                            b.last_recalculated_win_pool_id = i;
                            b.win = false;
                            b.available = asset(purchase_amount.amount * (PERCENT_PRECISION - sp -> loss_percent) / PERCENT_PRECISION, root_symbol) ;
                        });

                    };
                }
            }

    }


    std::vector <eosio::asset> calculate_forecast(account_name username, account_name host, uint64_t lepts, uint64_t pool_num){
        account_index accounts(_self, _self);
        auto acc = accounts.find(host);
        auto main_host = acc->get_active_host();
        

        balance_index balance (_self, username);
        rate_index rates (_self, main_host);
        spiral_index spiral (_self, main_host);

        auto root_symbol = acc->get_root_symbol();
        auto sp = spiral.find(0);

        std::vector<eosio::asset> forecasts;
        uint64_t lepts_forecast1;
        uint64_t lepts_forecast2;
        uint64_t lepts_forecast3;
        uint64_t lepts_forecast4;

        
        if (pool_num + 3 < sp->pool_limit){
            
            auto rate1 = rates.find(pool_num + 2);
            eosio::asset forecast1 = asset(lepts * rate1 -> sell_rate / LEPTS_PRECISION, root_symbol);
            
            forecasts.push_back(forecast1);
            lepts_forecast1 = lepts * rate1 -> sell_rate / rate1 -> buy_rate;

        } 

        if (pool_num + 5 < sp->pool_limit){
           
           auto rate2 = rates.find(pool_num + 4);
           eosio::asset forecast2 = asset(lepts_forecast1 * rate2 -> sell_rate / LEPTS_PRECISION, root_symbol);
           lepts_forecast2 = lepts_forecast1 * rate2 -> sell_rate / rate2 -> buy_rate;
           forecasts.push_back(forecast2);
        
        } 

        if (pool_num + 7 < sp->pool_limit){
            auto rate3 = rates.find(pool_num + 6);
            eosio::asset forecast3 = asset(lepts_forecast2 * rate3 -> sell_rate / LEPTS_PRECISION, root_symbol);
            lepts_forecast3 = lepts_forecast2 * rate3 -> sell_rate / rate3 -> buy_rate;
           
            forecasts.push_back(forecast3);

        }
        
        if (pool_num + 9 < sp->pool_limit){
            auto rate4 = rates.find(pool_num + 8);
            eosio::asset forecast4 = asset(lepts_forecast3 * rate4 -> sell_rate / LEPTS_PRECISION, root_symbol);
            forecasts.push_back(forecast4);
        }
        
        return forecasts;
        

    };


    void syswithdraw_action ( const syswithdraw &op){
        require_auth(op.host);
        auto host = op.host;
        
        account_index accounts(_self, _self);
        auto acc = accounts.find(host);
        auto main_host = acc->get_active_host();

        auto sys_balance_id = op.sbalanceid;
        auto username = op.username;

        sincome_index sincomes(_self, host);
        
        fee_index fee(_self,_self);
        auto f = fee.find(0);
        auto system_fee = f->systemfee;
        auto hoperator = acc -> hoperator;

        auto sinc = sincomes.find(sys_balance_id);
        eosio_assert(sinc -> withdrawed == false, "System balance is already withdrawed");
        
        eosio_assert(sinc -> id < acc -> current_cycle_num - 1, "Only closed cycles can be withdrawed");
        
        eosio::asset total_available = sinc -> available;
        eosio::asset system_amount = asset(system_fee * total_available.amount / PERCENT_PRECISION, _SYM);
        eosio::asset host_amount = total_available - system_amount / 2;
        eosio::asset operator_amount = host_amount;

        action(
            permission_level{ _self, N(active) },
            acc->root_token_contract, N(transfer),
            std::make_tuple( _self, host, host_amount, std::string("System Withdraw")) 
        ).send();


        if (system_amount.amount > 1){
            if (hoperator != 0 ){
                action(
                    permission_level{ _self, N(active) },
                    acc->root_token_contract, N(transfer),
                    std::make_tuple( _self, hoperator, operator_amount, std::string("System Withdraw")) 
                ).send();

                action(
                    permission_level{ _self, N(active) },
                    acc->root_token_contract, N(transfer),
                    std::make_tuple( _self, _dacomfee, host_amount, std::string("System Withdraw")) 
                ).send();

            } else {

                eosio::asset total_sys_amount = operator_amount + host_amount;

                action(
                    permission_level{ _self, N(active) },
                    acc->root_token_contract, N(transfer),
                    std::make_tuple( _self, _dacomfee, total_sys_amount, std::string("System Withdraw")) 
                ).send();

            }
        }

        sincomes.modify(sinc, _self,[&](auto &s){
            s.withdrawed = true;
        });

    }    



    void withdraw_action ( const withdraw &op){
        require_auth(op.username);
        
        auto username = op.username;
        auto balance_id = op.balance_id;
        auto host = op.host;

        account_index accounts(_self, _self);
        auto acc = accounts.find(host);
        account_name main_host = acc->get_active_host();
        auto root_symbol = acc->get_root_symbol();

        refresh_state(host);
        
        rate_index rates(_self, main_host);
        pool_index pools(_self, host);
        cycle_index cycles(_self, host);

        balance_index balance (_self, username);
        spiral_index spiral(_self, main_host);
        std::vector<eosio::asset> forecasts;
        
        
        auto sp = spiral.find(0);
        auto bal = balance.find(balance_id);
        auto pool = pools.find(bal->global_pool_id);
        auto cycle = cycles.find(pool -> cycle_num - 1);
        
        eosio_assert(bal->last_recalculated_win_pool_id == cycle->finish_at_global_pool_id, "Cannot withdraw not refreshed balance. Refresh Balance first and try again.");
        
        eosio_assert(! bal -> withdrawed, "Balance is already withdrawed");

        auto next_cycle = cycles.find(pool -> cycle_num);
        auto has_new_cycle = false;
        if (next_cycle != cycles.end())
            has_new_cycle = true;

        auto last_pool = pools.find(cycle -> finish_at_global_pool_id );
        auto rate = rates.find(last_pool -> pool_num - 1 );
        
        eosio_assert(last_pool -> remain_lepts <= pool->total_lepts, "Prevented withdraw. Only BP can restore this balance");
        
        uint64_t pools_in_cycle = cycle -> finish_at_global_pool_id - cycle -> start_at_global_pool_id + 1;
        
        
        if (((acc -> current_pool_num == pool -> pool_num ) && (acc -> current_cycle_num == pool -> cycle_num)) || \
            ((pool -> pool_num < 3) && (pools_in_cycle < 3)) || (has_new_cycle && (pool->pool_num == last_pool -> pool_num)))

        {

            if (bal->is_goal == false){
                action(
                    permission_level{ _self, N(active) },
                    acc->root_token_contract, N(transfer),
                    std::make_tuple( _self, username, bal -> purchase_amount, std::string("Withdraw")) 
                ).send();
            } else {
                adjust_goal_balance(bal->host, bal->goal_id, bal -> purchase_amount);
                delete_balance_id_from_goal(bal->host, bal->goal_id, bal->id);
            }



            if (pool -> pool_num < 3){

                pools.modify(pool, _self, [&](auto &p){
                    p.remain_lepts = std::min(pool-> remain_lepts + bal -> lept_for_sale, pool->total_lepts);
                }); 

            } else {

                pools.modify(last_pool, _self, [&](auto &p){
                    p.remain_lepts = std::min(last_pool-> remain_lepts + bal -> lept_for_sale, last_pool->total_lepts);
                });

            }

            balance.modify(bal, username, [&](auto &b){
                b.sold_amount = bal -> purchase_amount;
                b.date_of_sale = eosio::time_point_sec(now());
                b.lept_for_sale = 0; 
                b.withdrawed = true;
                b.available = asset(0, root_symbol);
                b.forecasts = forecasts;
            });

            //balance.erase(bal);
            
        } else  { 
            
            auto amount = bal -> available;
            if (bal->is_goal == false){
                action(
                    permission_level{ _self, N(active) },
                    acc->root_token_contract, N(transfer),
                    std::make_tuple( _self, username, amount, std::string("Withdraw")) 
                ).send();
            } else {
                adjust_goal_balance(bal->host, bal->goal_id, amount);
                delete_balance_id_from_goal(bal->host, bal->goal_id, bal->id);
            }
            
            uint64_t lepts_from_reserved;
            if (bal -> win == true){

                pools.modify(last_pool, _self, [&](auto &p){
                    auto converted_lepts = bal->lept_for_sale * rate -> sell_rate / rate -> buy_rate;
                    
                    p.creserved_lepts = (last_pool->creserved_lepts + converted_lepts) % LEPTS_PRECISION;
                    p.total_win_withdraw = last_pool -> total_win_withdraw + amount;
                    p.remain_lepts = std::min(last_pool -> remain_lepts + (last_pool->creserved_lepts + converted_lepts) / LEPTS_PRECISION * LEPTS_PRECISION, last_pool->total_lepts);
                   
                });
            }
            else {
                pools.modify(last_pool, _self, [&](auto &p){
                     p.total_loss_withdraw = last_pool -> total_loss_withdraw + amount;
                });
                
                
            }
                

            balance.modify(bal, username, [&](auto &b){
                b.sold_amount = bal -> available;
                b.date_of_sale = eosio::time_point_sec(now());
                b.lept_for_sale = 0;  
                b.withdrawed = true;
                b.available = asset(0, root_symbol);
                b.forecasts = forecasts;
            });

            //balance.erase(bal);
        }
    };

};

}