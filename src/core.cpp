using namespace eosio;
    
    
void unicore::add_balance(eosio::name payer, eosio::asset quantity, eosio::name contract){
    require_auth(payer);

    ubalances_index ubalances(_me, payer.value);
    
    auto balances_by_contract_and_symbol = ubalances.template get_index<"byconsym"_n>();
    auto contract_and_symbol_index = combine_ids(contract.value, quantity.symbol.code().raw());

    auto balance = balances_by_contract_and_symbol.find(contract_and_symbol_index);

    if (balance  == balances_by_contract_and_symbol.end()){
      ubalances.emplace(_me, [&](auto &b) {
        b.id = ubalances.available_primary_key();
        b.contract = contract;
        b.quantity = quantity;
      }); 
    } else {
      balances_by_contract_and_symbol.modify(balance, _me, [&](auto &b) {
        b.quantity += quantity;
      });
    };
  
}



void unicore::sub_balance(eosio::name username, eosio::asset quantity, eosio::name contract){
    ubalances_index ubalances(_me, username.value);
    
    auto balances_by_contract_and_symbol = ubalances.template get_index<"byconsym"_n>();
    auto contract_and_symbol_index = combine_ids(contract.value, quantity.symbol.code().raw());

    auto balance = balances_by_contract_and_symbol.find(contract_and_symbol_index);
    
    eosio::check(balance != balances_by_contract_and_symbol.end(), "Balance is not found");
    
    eosio::check(balance -> quantity >= quantity, "Not enought user balance for create order");

    if (balance -> quantity == quantity) {

      balances_by_contract_and_symbol.erase(balance);

    } else {

      balances_by_contract_and_symbol.modify(balance, _me, [&](auto &b) {
        b.quantity -= quantity;
      });  

    }
    
}

  uint64_t unicore::get_global_id(eosio::name key) {

    counts_index counts(_me, _me.value);
    auto count = counts.find(key.value);
    uint64_t id = 1;

    if (count == counts.end()) {
      counts.emplace(_me, [&](auto &c){
        c.key = key;
        c.value = id;
      });
    } else {
      id = count -> value + 1;
      counts.modify(count, _me, [&](auto &c){
        c.value = id;
      });
    }

    return id;

  }

    [[eosio::action]] void unicore::init(uint64_t system_income){
        require_auth(_me);

        gpercents_index gpercents(_me, _me.value);
        auto gsys_income = gpercents.find("system"_n.value);

        eosio::check(system_income <= 300000, "System income should be less or equal 30%");

        if (gsys_income == gpercents.end()){
            gpercents.emplace(_me, [&](auto &gp){
                gp.key = "system"_n;
                gp.value = system_income;
            });    
        } else {
            gpercents.modify(gsys_income, _me, [&](auto &g){
                g.value = system_income;
            });
        }
        
    }

    [[eosio::action]] void unicore::settype(eosio::name host, eosio::name type){
        require_auth(host);

        unicore::settype_static(host, type);

    }

    void unicore::settype_static(eosio::name host, eosio::name type){
        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Host is not found");
        
        if (type == "pot"_n) {
            eosio::check(acc-> sale_is_enabled == true, "Sale is not enabled");
        };

        accounts.modify(acc, host, [&](auto &a){
            a.type = type;
        });

    }



    /**
     * @brief Метод ручного пополнения целевого фонда сообщества. Жетоны, попадающие в целевой фонд сообщества, подлежат распределению на цели с помощью голосования участников по установленным правилам консенсуса.  
     * 
     * Метод может не использоваться, поскольку еще одним источником пополнения целевого фонда сообщества является установленный архитектором сообщества процент от финансового оборота ядра. 

     * Примеры:
     * Выпущен 1 млн жетонов, 90% из которых закреплены в целевом фонде, а 10% распределяются среди участников через прямое расределение любым способом (например, продажей). В этом случае, 90% жетонов должны быть помещены в целевой фонд, что гарантирует эмиссию жетонов на цели сообщества в зависимости от вращения ядра и установленного архитектором процента эмиссии при заранее известном общем количестве жетонов.

     * В случае, когда конфигурацией экономики не предусмотрено использование целевого фонда сообществ, или же когда для его пополнения используется только автоматический режим в зависимости от финансового оборота ядра, метод ручного пополнения может не использоваться. И в то же время он всегда доступен любому участнику сообщества простым переводом средств на аккаунт протокола с указанием суб-кода назначения и имени хоста сообщества.   

     * @param[in]  username  The username - имя пользователя, совершающего поолнение. 
     * @param[in]  host      The host - имя аккаунта хоста
     * @param[in]  amount    The amount - сумма для пополнения
     * @param[in]  code      The code - контракт токена, поступивший на вход.
     */

    void unicore::fund_emi_pool ( eosio::name host, eosio::asset amount, eosio::name code ){
        emission_index emis(_me, host.value);
        auto emi = emis.find(host.value);
        eosio::check(emi != emis.end(), "Emission pool for current host is not found");
        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Host is not found");

        auto root_symbol = acc->get_root_symbol();

        eosio::check(acc->root_token_contract == code, "Wrong token contract for this pool.");
        eosio::check ( amount.symbol == root_symbol, "Rejected. Invalid token for this pool.");
        emis.modify(emi, _me, [&](auto &e){
            e.fund = emi->fund + amount;
        });
    };
    
    /**
     * @brief внутренний метод эмиссии, который вызывается в момент распределения целевого фонда сообщества на цели участников. Вызывается в момент переключения порядкового номера пула на каждый последующий. Эмитирует в балансы активных целей сообщества установленный архитектором процент от свободного финансового потока из заранее накопленного в фонде целей сообщества. 
     * 
     * Фонд целей сообщества пополняется в момент вывода выигрыша каждым победителем или прямым пополнением любым участником. 
     * 
     * Фонд целей сообщества расходуется исходя из текущего финансового оборота в спирали при переключении раунда на каждый следующий. 
     * 
     * Распределение в момент переключения пулов определяется параметром процента эмиссии от живого остатка на продажу, что представляет собой  линию обратной связи от динамики вращения ядра.   
     *  
     * Таким образом, целевой фонд сообщества пополняется и расходуется согласно гибкому набору правил, обеспечивающих циркуляцию. 

     * 
     * @param[in]  host  The host - имя аккаунта хоста
     */
    // void unicore::priorenter(eosio::name username, eosio::name host, uint64_t balance_id){

    
    eosio::asset unicore::emit(eosio::name host, eosio::asset host_income, eosio::asset max_income){
        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        auto root_symbol = acc->get_root_symbol();

        eosio::name ahost = acc->get_ahost();
            
        emission_index emis(_me, host.value);
        auto emi = emis.find(host.value);
        eosio::asset emi_fund = emi -> fund;

        auto main_host = acc->get_ahost();
        
        // calculate sincome 
        print("on emission");
        eosio::asset total_dac_asset = asset(0, host_income.symbol);
        eosio::asset total_cfund_asset = asset(0, host_income.symbol);
        eosio::asset total_hfund_asset = asset(0, host_income.symbol);
        eosio::asset total_sys_asset = asset(0, host_income.symbol);
        
        if (host_income.amount > 0) {
            
            gpercents_index gpercents(_me, _me.value);
            uint64_t syspercent = gpercents.find("system"_n.value) -> value;

            // system_income = asset(host_income * syspercent / HUNDR_PERCENT);
            // host_income = host_income - system_income;

            // eosio::asset total_to_dacs = asset(host_income.amount * acc -> dacs_percent / HUNDR_PERCENT, root_symbol);
            // eosio::asset total_to_cfund = asset(host_income.amount * acc -> cfund_percent / HUNDR_PERCENT, root_symbol);

            double total_ref = (double)host_income.amount * (double)(acc->referral_percent) / (double)HUNDR_PERCENT;
            
            double total_ref_min_sys = total_ref * (HUNDR_PERCENT - syspercent) / HUNDR_PERCENT;
            double total_ref_sys = total_ref * syspercent / HUNDR_PERCENT;
        

            double total_dac = (double)host_income.amount * (double)(acc->dacs_percent) / (double)HUNDR_PERCENT;
            
            double total_dac_min_sys = total_dac * (HUNDR_PERCENT - syspercent) / HUNDR_PERCENT;
            double total_dac_sys = total_dac * syspercent / HUNDR_PERCENT;
        

            double total_cfund = (double)host_income.amount * (double)(acc->cfund_percent) / ((double)HUNDR_PERCENT);

            double total_cfund_min_sys = total_cfund * (HUNDR_PERCENT - syspercent) / HUNDR_PERCENT;
            double total_cfund_sys = total_cfund * syspercent / HUNDR_PERCENT;


            double total_hfund = (double)host_income.amount * (double)(acc->hfund_percent) / (double)HUNDR_PERCENT;
            
            double total_hfund_min_sys = total_hfund * (HUNDR_PERCENT - syspercent) / HUNDR_PERCENT;
            double total_hfund_sys = total_hfund * syspercent / HUNDR_PERCENT;
        

      
            double total_sys = total_dac_sys + total_cfund_sys + total_hfund_sys + total_ref_sys;

            total_dac_asset = asset((uint64_t)total_dac_min_sys, root_symbol);
            total_cfund_asset = asset((uint64_t)total_cfund_min_sys, root_symbol);
            total_hfund_asset = asset((uint64_t)total_hfund_min_sys, root_symbol);
            
            total_sys_asset = asset((uint64_t)total_sys, root_symbol);

            print("on total_dac_asset: ", total_dac_asset);
            print("on total_cfund_asset: ", total_cfund_asset);
            print("on total_sys_asset: ", total_sys_asset);

            if (total_dac_asset.amount > 0) {
                unicore::spread_to_dacs(host, total_dac_asset, acc -> root_token_contract);
            };
            
            if (total_cfund_asset.amount > 0) {
                emis.modify(emi, _me, [&](auto &e) {
                    emi_fund = emi_fund + total_cfund_asset;
                    e.fund = emi_fund;
                });
            };

            // if (total_hfund_asset.amount > 0){
            //     accounts.modify(acc, _me, [&](auto &a){
            //         a.quote_amount += total_hfund_asset;
            //     });
            // };
            

            if (total_sys_asset.amount > 0) {
                // Выплаты на системный аккаунт сообщества. 
                action(
                    permission_level{ _me, "active"_n },
                    "eosio"_n, "inprodincome"_n,
                    std::make_tuple( acc -> root_token_contract, total_sys_asset) 
                ).send();

            };

            sincome_index sincome(_me, host.value);
            
            auto sinc = sincome.find(acc->current_pool_id + 1);
            
            auto prev_sinc = sincome.find(acc -> current_pool_id);
        
            sincome.emplace(_me, [&](auto &s){
                s.max = max_income;
                s.pool_id = acc->current_pool_id + 1;
                s.ahost = main_host;
                s.cycle_num = acc->current_cycle_num;
                s.pool_num = acc->current_pool_num + 1;
                s.liquid_power = acc->total_shares;
                s.total = prev_sinc -> total + total_sys_asset + total_dac_asset + total_cfund_asset + total_hfund_asset;
                
                s.paid_to_sys = prev_sinc -> paid_to_sys + total_sys_asset;
                s.paid_to_dacs = prev_sinc -> paid_to_dacs + total_dac_asset;
                s.paid_to_cfund = prev_sinc -> paid_to_cfund + total_cfund_asset;
                s.paid_to_hfund = prev_sinc -> paid_to_hfund + total_hfund_asset;

                s.paid_to_refs = prev_sinc -> paid_to_refs + asset(0, root_symbol);
            }); 
        };



        //SPREAD TO GOALS        
        
        if (emi->percent > 0 && total_cfund_asset.amount > 0){
            goals_index goals(_me, host.value);
            
            eosio::asset em = unicore::adjust_goals_emission_pool(host, total_cfund_asset);  
            eosio::asset on_emission = emi_fund <= em ? emi_fund : em;
            
            eosio::asset by_votes = on_emission / 2;
            eosio::asset by_timestamp = on_emission - by_votes;

            eosio::asset fact_emitted = asset(0, on_emission.symbol);

            print("on_emission: ", on_emission);
            uint64_t limit = 30;

            if (by_votes.amount > 0) {
                print("by_votes: ", by_votes);
                
                std::vector<uint64_t> list_for_emit;
                uint64_t devider;

                auto idx_bv = goals.template get_index<"votes"_n>();
                
                auto i_bv = idx_bv.rbegin();

                uint64_t gcount = 0;
                uint64_t i = 0;
                while(i_bv != idx_bv.rend() && gcount != emi->gtop  && i <= limit) {
                    print("watch_id: ", i_bv -> id);
                    if((i_bv -> status == "validated"_n) && (i_bv -> target.amount > 0)){
                        print(" by votes goal->id: ", i_bv->id);
                        list_for_emit.push_back(i_bv->id);
                        gcount++;
                    }
                    i++;
                    i_bv++;
                }        

                
                devider = gcount < emi->gtop ? gcount : emi->gtop;
                
       
                if (devider != 0) {

                    eosio::asset each_goal_amount = asset((by_votes).amount / devider, root_symbol);

                    eosio::asset for_emit;
                    
                    if (each_goal_amount.amount > 0) {
                        for (auto id : list_for_emit) {
                            auto goal_for_emit = goals.find(id);
                            eosio::asset total_recieved = goal_for_emit->available + goal_for_emit->withdrawed;
                            eosio::asset until_fill = goal_for_emit->target - total_recieved;
                            
                            if ((each_goal_amount > until_fill) && (until_fill.amount > 0)) {
                                for_emit = until_fill; 
                            } else {
                                for_emit = each_goal_amount;
                            };
                    
                            fact_emitted += for_emit;

                            goals.modify(goal_for_emit, _me, [&](auto &g){
                                g.available = goal_for_emit->available + for_emit;
                                g.status = g.available + g.withdrawed >= g.target ? "filled"_n : "validated"_n;
                                
                                if (g.status == "filled"_n) {
                                    g.filled_votes = goal_for_emit -> positive_votes;
                                    g.positive_votes = 0;
                                };

                            });
                        }
                        
                    
                        
                  
                    }

                }

            }

            if (by_timestamp.amount > 0) {
                print(" by timestamp: ");

                std::vector<uint64_t> list_for_emit;
                uint64_t devider;

                auto index = goals.template get_index<"bystatus"_n>();
                
                auto goal = index.lower_bound("validated"_n.value);

                uint64_t gcount = 0;
                uint64_t i = 0;

                while(goal != index.end() && gcount != emi->gtop && goal -> status == "validated"_n && i <= limit) {
                    if((goal -> target.amount > 0)) {
                        list_for_emit.push_back(goal->id);
                        print(" by timestamp goal->id: ", goal->id);
                
                        gcount++;
                    }
                    i++;
                    goal++;
                }        

                
                devider = gcount < emi->gtop ? gcount : emi->gtop;
                
       
                if (devider != 0) {

                    eosio::asset each_goal_amount = asset((by_timestamp).amount / devider, root_symbol);

                    eosio::asset for_emit;
                    
                    if (each_goal_amount.amount > 0) {


                        for (auto id : list_for_emit) {
                            auto goal_for_emit = goals.find(id);
                            eosio::asset total_recieved = goal_for_emit->available + goal_for_emit->withdrawed;
                            eosio::asset until_fill = goal_for_emit->target - total_recieved;
                            
                            if ((each_goal_amount > until_fill) && (until_fill.amount > 0)) {
                                for_emit = until_fill; 
                            } else {
                                for_emit = each_goal_amount;
                            };
                    
                            fact_emitted += for_emit;

                            goals.modify(goal_for_emit, _me, [&](auto &g){
                                g.available = goal_for_emit->available + for_emit;
                                g.status = g.available + g.withdrawed >= g.target ? "filled"_n : "validated"_n;
                                
                                if (g.status == "filled"_n) {
                                    g.filled_votes = goal_for_emit -> positive_votes;
                                    g.positive_votes = 0;
                                };

                            });
                        }
                        
                    
                        
                  
                    }

                }


            }




            eosio::check(emi_fund >= fact_emitted, "Error on emission");
                        
            emis.modify(emi, _me, [&](auto &e){
                e.fund = emi_fund - fact_emitted;
            });

        }

        return total_hfund_asset;
    }

/**
 * @brief      Внутренний метод расчета пула эмиссии. Вызывается в момент распределения эмиссии на цели сообщества. Расчет объема эмиссии происходит исходя из параметра life_balance_for_sale завершенного пула, и процента эмиссии, установленного архитектором. Процент эмиссии ограничен от 0 до 1000% относительного живого остатка на продажу в каждом новом пуле. 
 * 
 * @param[in]  hostname  The hostname - имя аккаунта хоста
 *
 * @return     { description_of_the_return_value }
 */

    eosio::asset unicore::adjust_goals_emission_pool(eosio::name hostname, eosio::asset host_income){
        emission_index emis(_me, hostname.value);
        
        auto em = emis.find(hostname.value);
        eosio::asset for_emit;

        for_emit = asset(host_income.amount * em->percent / HUNDR_PERCENT, host_income.symbol );    
        
        return for_emit;
 };

/**
 * @brief      Метод приоритетного входа в новый цикл. Доступен к использованию только при условии наличия предыдущего цикла, в котором участник имеет проигравший баланс. Позволяет зайти частью своего проигравшего баланса одновременно в два пула - первый и второй нового цикла. Использование приоритетного входа возможно только до истечения времени приоритета, которое начинается в момент запуска цикла и продолжается до истечения таймера приоритета. 
 * 
 * Метод позволяет проигравшим балансам предыдущего цикла перераспределиться в новом цикле развития так, чтобы быть в самом начале вращательного движения и тем самым, гарантировать выигрыш. В случае успеха исполнения метода, пользователь обменивает один свой старый проигравший баланс на два новых противоположных цветов. 
 * 
 * В ходе исполнения метода решается арифметическая задача перераспределения, при которой вычисляется максимально возможная сумма входа для имеющегося баланса одновременно в два первых пула. Остаток от суммы, который невозможно распределить на новые пулы по причине нераздельности минимальной учетной единицы, возвращается пользователю переводом. 
 * 
 * Приоритетный вход спроектирован таким образом, то если все проигравшие участники одновременно воспользуются им, то в точности 50% внутренней учетной единицы для первого и второго пула будет выкуплено. 
 * 
 * TODO возможно расширение приоритетного входа до 100% от внутренней учетной единицы для первых двух пулов, а так же, продолжение приоритетного входа для всех последующих пулов.
 * 
 * 
 * @param[in]  op    The operation
 */

[[eosio::action]] void unicore::priorenter(eosio::name username, eosio::name host, uint64_t balance_id){
    
     unicore::refresh_state(host); 
     
     require_auth(username);

     pool_index pools(_me, host.value);
     balance_index balances(_me, host.value);
     cycle_index cycles(_me, host.value);
     account_index accounts(_me, _me.value);


     auto acc = accounts.find(host.value);
     auto main_host = acc->get_ahost();
     auto root_symbol = acc->get_root_symbol();

     spiral_index spiral(_me, main_host.value);
     auto sp = spiral.find(0);
     
     auto bal = balances.find(balance_id);
     eosio::check(bal != balances.end(), "Balance not exist");
     
     auto cycle = cycles.find(bal-> cycle_num - 1);
     
     eosio::check(bal -> host == host, "Impossible to enter with balance from another host");
     eosio::check(bal->cycle_num <= acc->current_cycle_num - 1, "Only participants from previous cycle can priority enter");
     eosio::check(bal->last_recalculated_win_pool_id == cycle->finish_at_global_pool_id, "Impossible priority enter with not refreshed balance");
     eosio::check(bal->win == 0, "Only lose balances can enter by priority");
     
     eosio::check(bal->available <= bal->purchase_amount, "Your balance have profit and you cannot use priority enter with this balance.");
     eosio::check(bal->available.amount > 0, "Cannot priority enter with a zero balance. Please, write to help-center for resolve it");

     tail_index tail(_me, host.value);
     auto end = tail.rbegin();
     
     uint64_t id = 0;

     if (end != tail.rend()) {
        id = end -> id + 1;
     };

     tail.emplace(_me, [&](auto &t) {
        t.id = id;
        t.username = username;
        t.enter_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
        t.amount = bal -> available;
        t.exit_pool_id = bal -> global_pool_id;
     });
     
    auto last_pool = pools.find(cycle->finish_at_global_pool_id);

    unicore::add_coredhistory(host, username, last_pool -> id, bal->available, "priority", "");

    pools.modify(last_pool, username, [&](auto &p){
        p.total_loss_withdraw = last_pool -> total_loss_withdraw + bal->available;
    });

    unicore::add_host_stat("withdraw"_n, username, host, bal->purchase_amount);
        

    balances.erase(bal);

}


[[eosio::action]] void unicore::pull(eosio::name host, eosio::name username, eosio::asset amount){
    // tail_index tail(_me, host.value);
    // auto end = tail.rbegin();

    // uint64_t id = 0;

    // if (end != tail.rend()) {
    //     id = end -> id + 1;
    // };

    // print("prior id: ", id);

    // tail.emplace(_me, [&](auto &t) {
    //     t.id = id;
    //     t.username = username;
    //     t.enter_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
    //     t.amount = amount;
    // });
    
};


[[eosio::action]] void unicore::exittail(eosio::name username, eosio::name host, uint64_t id) {
    require_auth(username);
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    tail_index tails(_me, host.value);
    
    auto utail = tails.find(id);

    
    eosio::check(utail != tails.end(), "Tail object is not found");
    eosio::check(utail -> username == username, "Missing requiring auth");
    
     action(
        permission_level{ _me, "active"_n },
        acc->root_token_contract, "transfer"_n,
        std::make_tuple( _me, username, utail -> amount, std::string("Tail back")) 
    ).send();

    tails.erase(utail);

};

void unicore::cut_tail(uint64_t current_pool_id, eosio::name host) {
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    eosio::name main_host = acc->get_ahost();
    auto root_symbol = acc->get_root_symbol();
    
    pool_index pools(_me, host.value);
    print("ON CUT", current_pool_id);
    auto pool = pools.find(current_pool_id);
    print("ON CUT2", pool->id);
    rate_index rates(_me, main_host.value);
    auto rate = rates.find(pool -> pool_num - 1);

    spiral_index spiral(_me, main_host.value);
    auto sp = spiral.find(0);

    tail_index tails(_me, host.value);
    

    auto user_tail = tails.begin();
    
    eosio::asset max_amount = asset(uint64_t((double)pool -> remain.amount * 0.5), root_symbol);

    eosio::asset filled = asset(0, root_symbol);

    bool more = true;
    uint64_t count = 0;


    //not more then 25 users get in pool
    while (user_tail != tails.end() && more && count < 25) {

        if (filled + user_tail -> amount < max_amount) {
            
            filled += user_tail -> amount;
            
            uint128_t dquants = uint128_t(sp -> quants_precision * (uint128_t)user_tail -> amount.amount / (uint128_t)rate -> buy_rate);
            
            uint64_t quants = dquants;
            print(" FULL FILL : ", user_tail -> id);
            print(" user: ", (name{user_tail->username}.to_string()));
            print(" amount: ", user_tail -> amount);
            print(" max_to_fill: ", max_amount);
            print(" total_filled: ", filled);

            unicore::fill_pool(user_tail -> username, host, quants, user_tail -> amount, current_pool_id);

            user_tail = tails.erase(user_tail);


        } else {

            eosio::asset user_diff = max_amount - filled;
            
            filled = max_amount;

            tails.modify(user_tail, _me, [&](auto &t){
                t.amount -= user_diff;
            });

            uint128_t dquants = uint128_t(sp -> quants_precision * (uint128_t)user_diff.amount / (uint128_t)rate -> buy_rate);
            uint64_t quants = dquants;

            unicore::fill_pool(user_tail->username, host, quants, user_diff, current_pool_id);

            print(" PART FILL: ", user_tail -> id);
            print(" user: ", (name{user_tail->username}.to_string()));
            print(" quants: ", quants);
            print(" amount: ", user_diff);
            print(" max_amount: ", max_amount);
            print(" total_filled: ", filled);
            

            more = false;
        };

        count++;

        if (count >= 25){
            print("LIMIT; STOP");
        }    
    }

}

/**
 * @brief Получение параметров нового цикла. Внутренний метод, используемый для создания записей в базе данных, необходимых для запуска нового цикла.      
 *
 * @param[in]  host       The host - имя аккаунта хоста
 * @param[in]  main_host  The main host - указатель на имя аккаунта второстепенного хоста, содержающего измененные параметры вращения  (если такой есть)
 */
void improve_params_of_new_cycle (eosio::name host, eosio::name main_host){
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    auto root_symbol = acc->get_root_symbol();
    
    sincome_index sincome(_me, host.value);
    cycle_index cycles(_me, host.value);
    
    auto last_cycle = cycles.find(acc->current_cycle_num - 1);

    cycles.emplace(_me, [&](auto &c){
        c.id = cycles.available_primary_key();
        c.ahost = main_host;
        c.start_at_global_pool_id = last_cycle->finish_at_global_pool_id + 1; 
        c.finish_at_global_pool_id = last_cycle->finish_at_global_pool_id + 2; 
        c.emitted = asset(0, root_symbol);   
    });

    spiral_index spiral(_me, main_host.value);
    auto sp = spiral.find(0);

    accounts.modify(acc, _me, [&](auto &dp){
        dp.current_pool_id  = last_cycle->finish_at_global_pool_id + 1;
        dp.cycle_start_id = dp.current_pool_id;
        dp.current_cycle_num = acc->current_cycle_num + 1;
        dp.current_pool_num  = 1;
        dp.sale_shift = sp -> base_rate;
        // dp.priority_flag = true;       
    });
}
 
/**
 * @brief Внутренний метод установки первых пулов нового цикла. 
 *
 * @param[in]  parent_host  The parent host
 * @param[in]  main_host    The main host
 * @param[in]  root_symbol  The root symbol
 */

void emplace_first_pools(eosio::name parent_host, eosio::name main_host, eosio::symbol root_symbol, eosio::time_point_sec start_at){
    
    spiral_index spiral(_me, main_host.value);
    auto sp = spiral.find(0);
    eosio::check(sp != spiral.end(), "Protocol is not found. Set parameters at first");
    sincome_index sincome(_me, parent_host.value);
    print("on emplace");
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(parent_host.value);
    cycle_index cycles(_me, parent_host.value);
    pool_index pools(_me, parent_host.value);

    rate_index rates(_me, main_host.value);
    
    auto rate = rates.find(0);
    auto next_rate = rates.find(1);
    auto next_next_rate = rates.find(2);

    auto available_id = pools.available_primary_key();

    uint64_t total_quants = sp->size_of_pool * sp -> quants_precision;
    print("on emplace2");
    pools.emplace(_me, [&](auto &p){
        p.id = available_id;
        p.ahost = main_host;
        p.total_quants = total_quants;
        p.creserved_quants = 0;
        p.remain_quants = total_quants;
        p.quant_cost = asset(rate->buy_rate, root_symbol);

        p.pool_cost = asset(sp->size_of_pool * rate->buy_rate, root_symbol);
        p.filled = asset(0, root_symbol);
        p.remain = p.pool_cost;
        p.filled_percent = 0;
        
        p.total_win_withdraw = asset(0, root_symbol);
        p.total_loss_withdraw = asset(0, root_symbol);
        p.pool_num = 1;
        p.cycle_num = acc->current_cycle_num;
        p.pool_started_at = start_at;
        p.priority_until = eosio::time_point_sec(0);
        p.pool_expired_at = eosio::time_point_sec (-1);
        p.color = "white";
    });
    print("on emplace3");
    
    sincome.emplace(_me, [&](auto &s){
        s.max = rate -> system_income;
        s.pool_id = available_id;
        s.ahost = main_host;
        s.cycle_num = acc->current_cycle_num;
        s.pool_num = 1;
        s.liquid_power = acc->total_shares;
        s.total = asset(0, root_symbol);
        s.paid_to_sys = asset(0, root_symbol);
        s.paid_to_dacs = asset(0, root_symbol);
        s.paid_to_cfund = asset(0, root_symbol);
        s.paid_to_hfund = asset(0, root_symbol);
        s.paid_to_refs = asset(0, root_symbol);
    }); 

    unicore::cut_tail(available_id, parent_host);
    
    print("on emplace4");

    unicore::change_bw_trade_graph(parent_host, available_id, acc->current_cycle_num, 1, rate->buy_rate, next_rate->buy_rate, total_quants, total_quants, "white");
    
    pools.emplace(_me, [&](auto &p){
        p.id = available_id + 1;
        p.ahost = main_host;
        p.total_quants = total_quants;
        p.creserved_quants = 0;
        p.remain_quants =total_quants;
        p.quant_cost = asset(next_rate->buy_rate, root_symbol);

        p.pool_cost = asset(sp->size_of_pool * next_rate->buy_rate, root_symbol);
        p.filled = asset(0, root_symbol);
        p.remain = p.pool_cost;
        p.filled_percent = 0;

        p.color = "black";
        p.cycle_num = acc->current_cycle_num;
        p.pool_num = 2;
        p.pool_started_at = start_at;
        p.priority_until = eosio::time_point_sec(0);
        p.pool_expired_at = eosio::time_point_sec (-1);
        p.total_win_withdraw = asset(0, root_symbol);
        p.total_loss_withdraw = asset(0, root_symbol);
    }); 

    sincome.emplace(_me, [&](auto &s){
        s.max = next_rate -> system_income;
        s.pool_id = available_id + 1;
        s.ahost = main_host;
        s.cycle_num = acc->current_cycle_num;
        s.pool_num = 2;
        s.liquid_power = acc->total_shares;
        s.total = asset(0, root_symbol);
        s.paid_to_sys = asset(0, root_symbol);
        s.paid_to_dacs = asset(0, root_symbol);
        s.paid_to_cfund = asset(0, root_symbol);
        s.paid_to_hfund = asset(0, root_symbol);
        s.paid_to_refs = asset(0, root_symbol);
    }); 
    
    // if (acc -> type== "bw"_n){
    unicore::change_bw_trade_graph(parent_host, available_id + 1, acc->current_cycle_num, 2, next_rate->buy_rate, next_next_rate->buy_rate, total_quants, total_quants, "black");
    // }
    powerstat_index powerstats(_me, main_host.value);
    auto powerstat = powerstats.rbegin();

    action(
        permission_level{ _me, "active"_n },
        "eosio"_n, "pushupdate"_n,
        std::make_tuple( main_host , acc -> current_cycle_num, available_id, eosio::time_point_sec (-1), eosio::time_point_sec (-1)) 
    ).send();
    
}


void unicore::change_bw_trade_graph(eosio::name host, uint64_t pool_id, uint64_t cycle_num, uint64_t pool_num, uint64_t buy_rate, uint64_t next_buy_rate, uint64_t total_quants, uint64_t remain_quants, std::string color){
    bwtradegraph_index bwtradegraph(_me, host.value);

    uint64_t open = buy_rate; 
    uint64_t low = buy_rate;
    uint64_t high = next_buy_rate;
    
    double close = open + (total_quants - remain_quants) / (double)total_quants * (high - low);
    
    auto bwtrade_obj = bwtradegraph.find(pool_id);

    if (bwtrade_obj == bwtradegraph.end()){
        bwtradegraph.emplace(_me, [&](auto &bw){
            bw.pool_id = pool_id;
            bw.cycle_num = cycle_num;
            bw.pool_num = pool_num;
            bw.open = open;
            bw.low = low;
            bw.high = high;
            bw.close = (uint64_t)close;
            bw.is_white = color == "white" ? true : false;
        });

    } else {
        bwtradegraph.modify(bwtrade_obj, _me, [&](auto &bw){
            bw.close = (uint64_t)close;
        });

    }
}

/**
 * @brief      Внутрений метод запуска нового цикла. 
 * Вызывается при достижении одного из множества условий. Вызывает расчет показательной статистики цикла и установку новых пулов. Если установлен флаг переключения на дочерний хост, здесь происходит замена основного хоста и снятие флага. Дочерний хост хранит в себе измененные параметры финансового ядра. 

 * @param[in]  host  The host
 */
void start_new_cycle ( eosio::name host ) {
    account_index accounts(_me, _me.value);
    cycle_index cycles(_me, host.value);
            
    auto acc = accounts.find(host.value);
    eosio::name main_host = acc->get_ahost();
    eosio::name last_ahost = acc->ahost;
            
    auto root_symbol = acc->get_root_symbol();
    eosio::time_point_sec start_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());

    if (acc->need_switch) {

        main_host = (acc->chosts).back();
        
        auto chosts = acc -> chosts;

        chosts.erase(chosts.begin());

        accounts.modify(acc, _me, [&](auto &a){
            a.need_switch = false;
            a.ahost = main_host;
            a.chosts = chosts;
        });

        improve_params_of_new_cycle(host, main_host);

        emplace_first_pools(host, main_host, root_symbol, start_at);

    } else {
        rate_index rates(_me, main_host.value);
        spiral_index spiral(_me, main_host.value);
        
        auto cycle = cycles.find(acc-> current_cycle_num - 1);
        auto rate = rates.find(0);
        auto next_rate = rates.find(1);
        auto sp = spiral.find(0);
        
        pool_index pools(_me, host.value);
        uint64_t available_id = pools.available_primary_key();

        print("on new3");
        improve_params_of_new_cycle(host, main_host);
        print("on new4");
        emplace_first_pools(host, main_host, root_symbol, start_at);
        print("on new5");
    }
        
        unicore::refresh_state(host);  


    };



/**
 * @brief      Внутренний метод открытия следующего пула
 * Вызывается только при условии полного выкупа всех внутренних учетных единиц предыдущего пула. 
 *
 * @param[in]  host  The host
 */
void next_pool( eosio::name host) {
    account_index accounts(_me, _me.value);
    
    auto acc = accounts.find(host.value);
    auto main_host = acc->get_ahost();
    
    sincome_index sincome(_me, host.value);
    cycle_index cycles(_me, host.value);
    pool_index pools(_me, host.value);

    //PRINT
    print((name{main_host}.to_string()));

    rate_index rates(_me, main_host.value);
    spiral_index spiral(_me, main_host.value);
    

    auto root_symbol = acc->get_root_symbol();
    auto pool = pools.find(acc -> current_pool_id);
    auto cycle = cycles.find(acc -> current_cycle_num - 1);
    
    auto sp = spiral.find(0);
    
    uint128_t dreserved_quants = 0;
    uint64_t reserved_quants = 0;

    print("on next pool0");
    //Если первые два пула не выкуплены, это значит, 
    //не все участники воспользовались приоритетным входом, и пул добавлять не нужно. 

    if (acc -> current_pool_num > 1) {
        print("z", acc -> current_pool_num);
        auto ratem1 = rates.find(acc-> current_pool_num - 1);
        auto rate = rates.find(acc-> current_pool_num);
        auto ratep1 = rates.find(acc-> current_pool_num + 1);

        eosio::asset host_income = asset(0, root_symbol);
        eosio::asset system_income = asset(0, root_symbol);

        print("x", rate -> system_income, ratem1->system_income);
        
        if (rate -> system_income > ratem1 -> system_income) {

            host_income = rate -> system_income - ratem1 -> system_income;
            
        }
        print("on next pool00");
        cycles.modify(cycle, _me, [&](auto &c ){
            c.finish_at_global_pool_id = cycle -> finish_at_global_pool_id + 1;
        });
        
        eosio::asset quote_amount = unicore::emit(host, host_income, rate -> system_income);
        //TODO
        //add_convert_rate here
        //
        
        accounts.modify(acc, _me, [&](auto &dp){
           dp.current_pool_num = pool -> pool_num + 1;
           dp.current_pool_id  = pool -> id + 1;
           dp.quote_amount += quote_amount;
           dp.sale_shift = rate -> buy_rate;// / sp -> base_rate;
           
           // print("new sale shift: ", dp.sale_shift);
        });

        print("on next pool000");
        auto prev_prev_pool = pools.find(acc -> current_pool_id - 2);
        
        dreserved_quants = (prev_prev_pool -> total_quants - prev_prev_pool -> remain_quants ) / sp -> quants_precision  * rate -> sell_rate / rate -> buy_rate * sp -> quants_precision;

        reserved_quants = uint64_t(dreserved_quants);

        // double creserved_quants = prev_prev_pool -> creserved_quants * rate -> sell_rate / rate -> buy_rate;
        double creserved_quants = 0;
        
        eosio::check(pool -> pool_num + 1 <= sp -> pool_limit, "Please, write to help-center for check why you cannot deposit now");
    
        uint64_t pool_id = pools.available_primary_key();
        eosio::time_point_sec expired_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch() + sp->pool_timeout);

        pools.emplace(_me, [&](auto &p){
            p.id = pool_id;
            p.ahost = main_host;
            p.total_quants = sp->size_of_pool * sp -> quants_precision;
            p.creserved_quants = uint64_t(creserved_quants);
            p.remain_quants = p.total_quants - reserved_quants;
            p.quant_cost = asset(rate->buy_rate, root_symbol);
            p.color = prev_prev_pool -> color; 
            p.cycle_num = pool -> cycle_num;
            p.pool_num = pool -> pool_num + 1;
            p.pool_started_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
            p.priority_until = eosio::time_point_sec(0);
            p.pool_expired_at = expired_at;
            p.total_win_withdraw = asset(0, root_symbol);
            p.total_loss_withdraw = asset(0, root_symbol);

            p.pool_cost = asset(sp->size_of_pool * rate->buy_rate, root_symbol);
            p.filled = asset(reserved_quants / sp -> quants_precision * rate->buy_rate, root_symbol);
            p.remain = p.pool_cost - p.filled;
            p.filled_percent = reserved_quants * HUNDR_PERCENT / p.total_quants;
            

            unicore::change_bw_trade_graph(main_host, p.id, p.cycle_num, p.pool_num, rate->buy_rate, ratep1->buy_rate, p.total_quants, p.remain_quants, p.color);
        });
        // print("on next pool1");
        // // unicore::cut_tail(pool_id, host);
        // print("on next pool2");
        powerstat_index powerstats(_me, main_host.value);
        auto powerstat = powerstats.rbegin();

        action(
            permission_level{ _me, "active"_n },
            "eosio"_n, "pushupdate"_n,
            std::make_tuple( host , acc -> current_cycle_num, pool_id, expired_at, powerstat -> window_closed_at) 
        ).send();
    } else {
        //Если это стартовые пулы, то только смещаем указатель
        auto rate = rates.find(acc-> current_pool_num);
        // print("ACCOUNT POOL BEFORE CHANGE: ", acc -> current_pool_id);
        accounts.modify(acc, _me, [&](auto &a){
           a.current_pool_num = pool -> pool_num + 1;
           a.current_pool_id  = pool -> id + 1;
           // a.sale_shift = rate -> buy_rate;// / sp -> base_rate;
           // print("SALE_SHIFT: ", a.sale_shift);
           // a.priority_flag = false;     
        });

        unicore::cut_tail(pool -> id + 1, host);
        powerstat_index powerstats(_me, main_host.value);
        auto powerstat = powerstats.rbegin();
        action(
            permission_level{ _me, "active"_n },
            "eosio"_n, "pushupdate"_n,
            std::make_tuple( host , acc -> current_cycle_num, pool -> id + 1, eosio::time_point_sec (-1), powerstat -> window_closed_at) 
        ).send();

    }   

    
         
};

[[eosio::action]] void unicore::fixs(eosio::name host, eosio::name username){
    require_auth(_me);
    // ++ удалить hosts
    // ++ удалить rate
    // ++ удалить ahost
    // ++ удалить coreahost
    // ++ удалить cycles
    // установить силу 8 значков
    // удалить цели
    // удалить действия
    // 
    // spiral_index spiral(_me, host.value);
    // auto sp = spiral.find(0);
    
    // power3_index power(_me, host.value);
    // auto pow = power.find(username.value);
    
    // goldens_index goldens(_me, host.value);
    // auto golden = goldens.find(host.value);


    // if (pow -> staked > 0) {
    //     power.modify(pow, _me, [&](auto &p){
    //         p.staked = 0;
    //         p.frozen = pow -> power;
    //     });

    //     if (golden == goldens.end()) {

    //         goldens.emplace(_me, [&](auto &g){
    //             g.host = host;
    //             g.total_golden = pow -> power;
    //         });

    //     } else {

    //         goldens.modify(golden, _me, [&](auto &g){
    //             g.total_golden += pow -> power;
    //         });
    //     };
    // };
    // while (p != power.end()) {
    //     power.modify(p, [&](auto &pp){
    //         pp.staked = 0;
    //         pp.frozen = p -> power * sp -> quants_precision;
    //     });

    //     p++;
    // }
    // auto p = power.find(host.value);
    // power.erase(p);

    // usdtwithdraw_index uwithdraw(_me, host.value);
    // auto uw = uwithdraw.find(pool_num);
    // uwithdraw.erase(uw);

    // refbalances_index refbalances(_me, referer.value);
    
    // refbalances2_index refbalances2(_me, host.value);
    // auto rb = refbalances2.find("core"_n.value)    ;         

    // refbalances2.erase(rb);

    //------- сделано
      
    // goals_index goals(_me, host.value);
    // auto goal = goals.begin();
    
    // while (goal != goals.end()) {
    //     goal = goals.erase(goal);
    // }

    //  hoststat_index hoststat(_me, host.value);
    //  auto hstat = hoststat.begin();
    // while (hstat != hoststat.end()) {
    //     hstat = hoststat.erase(hstat);
    // }


    // tasks_index tasks(_me, host.value);
    // auto task = tasks.begin();
    
    // while (task != tasks.end()) {
    //     task = tasks.erase(task);
    // }

    // reports_index reports(_me, host.value);
    // auto report = reports.begin();

    // while (report != reports.end()) {
    //     report = reports.erase(report);
    // }

    // vesting_index vests(_me, host.value);
    // auto v = vests.begin();
    // while (v != vests.end()) {
    //     v = vests.erase(v);
    // }
    //   rate_index rates(_me, host.value);
      
    //   auto rate = rates.begin();
      
    //   if (rate != rates.end()) {
    //     rates.erase(rate);  
    //   };

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    // if (acc -> total_shares < sp -> quants_precision){
        // accounts.modify(acc, _me, [&](auto &a){
        //     a.total_shares = acc -> total_shares / 100;
        // });
    // };
    // accounts2_index2 accounts2(_me, _me.value);
    // accounts2.emplace(_me, [&](auto &a2) {
    //     username = acc -> username;
    //     registered_at = acc -> registered_at;
    //     architect = acc -> architect;
    //     hoperator = acc -> hoperator;
    //     type = acc -> type;
    //     chat_mode = acc -> chat_mode;
    //     consensus_percent = acc -> consensus_percent;
    //     referral_percent = acc -> referral_percent;
    //     dacs_percent = acc -> dacs_percent;
    //     cfund_percent = acc -> cfund_percent;
    //     hfund_percent = acc -> hfund_percent;
    //     sys_percent = acc -> sys_percent;
    //     levels = acc -> levels;
    //     gsponsor_model = acc -> gsponsor_model;
    //     direct_goal_withdraw = acc -> direct_goal_withdraw;
    //     dac_mode = acc -> dac_mode;
    //     total_dacs_weight = acc -> total_dacs_weight;
        
    //     ahost = acc -> ahost;
    //     chosts = acc -> chosts;
        
    //     sale_is_enabled = acc -> sale_is_enabled;
    //     sale_mode = acc -> sale_mode;
    //     sale_token_contract = acc -> sale_token_contract;
    //     asset_on_sale = acc -> asset_on_sale;
    //     asset_on_sale_precision = acc -> asset_on_sale_precision;
    //     asset_on_sale_symbol = acc -> asset_on_sale_symbol;
    //     sale_shift = acc -> sale_shift;
        
    //     //Метод добавления хоста в фонд
    //     //Метод включения сейла хостом
        
    //     bool non_active_chost = false;
    //     bool need_switch = false;

    //     uint64_t fhosts_mode;
    //     std::vector<eosio::name> fhosts;
        

    //     std::string title;
    //     std::string purpose;
    //     bool voting_only_up = false;
    //     eosio::name power_market_id;
    //     uint64_t total_shares;
    //     eosio::asset quote_amount;
    //     eosio::name quote_token_contract;
    //     std::string quote_symbol;
    //     uint64_t quote_precision;
    //     eosio::name root_token_contract;
    //     eosio::asset root_token;
    //     std::string symbol;
    //     uint64_t precision;
    //     eosio::asset to_pay;
    //     bool payed = false;
        
    //     uint64_t cycle_start_id = 0;
    //     uint64_t current_pool_id = 0;
    //     uint64_t current_cycle_num = 1;
    //     uint64_t current_pool_num = 1;
    //     bool parameters_setted = false;
    //     bool activated = false;
        
    //     bool priority_flag = false;

    //     uint64_t total_goals = 0;
    //     uint64_t achieved_goals = 0;
    //     uint64_t total_tasks = 0;
    //     uint64_t completed_tasks = 0;
    //     uint64_t total_reports = 0;
    //     uint64_t approved_reports = 0;
        

    //     std::string meta;
        
    // })

    pool_index pools(_me, host.value);
    auto pool = pools.find(acc -> current_pool_id);
    
    pools.modify(pool, _me, [&](auto &p){
        p.pool_expired_at = eosio::time_point_sec(0);
    });
    
    // account3_index accounts3(_me, _me.value);
    // auto acc3 = accounts3.find(host.value);
    
    // if (acc3 != accounts3.end()) {
    //     accounts3.erase(acc3);    
    // };
    
    //   if (acc != accounts.end()){
    //     accounts.erase(acc);  
    //   }
    //   market_index market(_me, host.value);
    //   auto itr = market.find(0);
      
    //   if (itr != market.end()) {
    //     market.erase(itr);  
    //   }
      

    //   ahosts_index coreahosts(_me, _me.value);
    //   auto corehost = coreahosts.find(host.value);
      
    //   if (corehost != coreahosts.end()){
    //     coreahosts.erase(corehost);  
    //   }
      
      

    //   ahosts_index ahosts(_me, host.value);
    //   auto ahost = ahosts.find(host.value);

    //   if (ahost != ahosts.end()) {
    //     ahosts.erase(ahost);  
    //   }
      
      

    //   cycle_index cycles(_me, host.value);

    //   auto cycle = cycles.find(0);

    //   if (cycle != cycles.end()) {
    //     cycles.erase(cycle);
    //   }
      

    // spiral_index spiral(_me, host.value);
    // auto sp = spiral.find(0);
    // spiral.modify(sp, _me, [&](auto &sp){
    //     sp.pool_timeout = 2592000;
    // });

    
    // spiral.erase(sp);
    

    // bwtradegraph_index bwtradegraph(_me, host.value);
    // auto it = bwtradegraph.begin();
    // while (it != bwtradegraph.end()) {
    //     it = bwtradegraph.erase(it);
    // };


    //  pool_index pools(_me, host.value);
    //  auto it2 = pools.begin();
 
    //  while (it2 != pools.end()) {
    //     it2 = pools.erase(it2);
    
    // };



    // spiral2_index spiral2(_me, host.value);
    //   auto sp2 = spiral2.find(0);
    //   if (sp2 != spiral2.end()){
    //     spiral2.erase(sp2);  
    //   };
      
    // coredhistory_index coredhistory(_me, host.value);
    // auto cored = coredhistory.begin();

    // while (cored != coredhistory.end()) {
    //     cored = coredhistory.erase(cored);
    // }    


      // emission_index emis(_me, host.value);
      // auto emi = emis.find(host.value);
      
      // if (emi != emis.end()) {
      //   emis.erase(emi);  
      // };
      // emis.emplace(_me, [&](auto &e){
      //   e.host = host;
      //   e.percent = 1000000;
      //   e.gtop = 0;
      //   e.fund = asset(0, _SYM);

      // });


    //   sincome_index sincome(_me, host.value);
    //   auto s1 = sincome.begin();


    // while (s1 != sincome.end()) {

    //     s1 = sincome.erase(s1);

    // }  




     // ____ не сделано
     // 
    // goals 
    // tasks
    // reports
    // 

    // accounts.modify(acc, _me, [&](auto &a){
    //     a.total_dacs_weight = 2;
    // });

}


[[eosio::action]] void unicore::setstartdate(eosio::name host, eosio::time_point_sec start_at){
    require_auth(host);

    account_index hosts(_me, _me.value);
    pool_index pools(_me, host.value);

    auto acc = hosts.find(host.value);
    
    eosio::check(acc -> current_pool_num == 1, "Only on the first pool start date can be changed");
    auto pool1 = pools.find(acc -> current_pool_id);
    auto pool2 = pools.find(acc-> current_pool_id + 1);

    pools.modify(pool1, host, [&](auto &p){
        p.pool_started_at = start_at;
    });

    pools.modify(pool2, host, [&](auto &p){
        p.pool_started_at = start_at;
    });
}

/**
 * @brief      Публичный метод запуска хоста
 * Метод необходимо вызвать для запуска хоста после установки параметров хоста. Добавляет первый цикл, два пула, переключает демонастративный флаг запуска и создает статистические объекты. Подписывается аккаунтом хоста.  
 * @auth    host | chost
 * @ingroup public_actions
 * @param[in]  host        имя аккаунта текущего хоста
 * @param[in]  chost       имя аккаунта дочерного хоста
   
 */
[[eosio::action]] void unicore::start(eosio::name host, eosio::name chost) {
    if (host == chost)
        require_auth(host);
    else 
        require_auth(chost);

    auto main_host = host;

    account_index accounts(_me, _me.value);
    gpercents_index gpercents(_me, _me.value);
    auto syspercent = gpercents.find("system"_n.value);
    eosio::check(syspercent != gpercents.end(), "Contract is not active");

    sincome_index sincome (_me, main_host.value);
    cycle_index cycles (_me, host.value);

    auto account = accounts.find(main_host.value);

    eosio::check(account != accounts.end(), "Account is not upgraded to Host. Please update your account and try again.");
    eosio::check(account->payed == true, "Host is not payed and can not start");

    auto root_symbol = account -> get_root_symbol();
    eosio::check(account->parameters_setted == true, "Cannot start host without setted parameters");
    eosio::time_point_sec start_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
    
    auto chosts = account->chosts;

    if (chosts.begin() == chosts.end()) {
        eosio::check(account->activated == false, "Protocol is already active and cannot be changed now.");
        
        accounts.modify(account, _me, [&](auto &a){
            a.activated = true;
            a.total_shares += 1;
        });

        cycles.emplace(_me, [&](auto &c){
            c.ahost = main_host;
            c.id = cycles.available_primary_key();
            c.start_at_global_pool_id = 0;
            c.finish_at_global_pool_id = 1;
            c.emitted = asset(0, root_symbol);
        });

        powerstat_index powerstat(_me, host.value);
        auto pstat = powerstat.find(0);
        
        powerstat.emplace(_me, [&](auto &ps){
            ps.id = powerstat.available_primary_key();
            ps.window_open_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
            ps.window_closed_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + _WINDOW_SECS);
            ps.liquid_power = 1;
            ps.total_available = asset(0, root_symbol);
            ps.total_remain = asset(0, root_symbol);
            ps.total_distributed = asset(0, root_symbol);
            ps.total_partners_distributed = asset(0, root_symbol);
            ps.total_partners_available = asset(0, root_symbol);
        }); 
        
        powerlog_index powerlogs(_me, host.value);
        powerlogs.emplace(_me, [&](auto &pl){
          pl.id = unicore::get_global_id("powerlog"_n);
          pl.host = host;
          pl.username = host;
          pl.window_id = 0;
          pl.power = 1;
          pl.available = asset(0, root_symbol);
          pl.distributed_to_partners = asset(0, root_symbol);
          pl.updated = false;
        });

        emplace_first_pools(host, main_host, root_symbol, start_at);
        
        power3_index power(_me, host.value);
        auto pexist = power.find(account -> architect.value);

        power.emplace(_me, [&](auto &p) {
          p.username = account -> architect;
          p.power = 1;
          p.staked = 1;    
        });

        
    } else {
        
        eosio::check(account->parameters_setted == true, "Cant start branch without setted parameters");
        main_host = chosts.back();
        eosio::check(account->ahost != main_host, "Protocol is already active and cannot be changed now.");
        eosio::check(main_host == chost, "Wrong last non-active child host");
        
        accounts.modify(account, _me, [&](auto &a){
            a.need_switch = true;
            a.non_active_chost = false;
        });
    };
}


/**
 * @brief      Публичный метод установки параметров протокола двойной спирали
 *  Вызывается пользователем после базовой операции создания хоста и проведения оплаты. Так же вызывается при установке параметров дочернего хоста. Содержит алгоритм финансового ядра. Производит основные расчеты таблиц курсов и валидирует положительность бизнес-дохода. 
 *  

    
 * @param[in]  op    The operation
 */
[[eosio::action]] void unicore::setparams(eosio::name host, eosio::name chost, uint64_t size_of_pool,
            uint64_t quants_precision, uint64_t overlap, uint64_t profit_growth, uint64_t base_rate,
            uint64_t loss_percent, uint64_t compensator_percent, uint64_t pool_limit, uint64_t pool_timeout, uint64_t priority_seconds)
{
    if (host == chost)
        require_auth(host);
    else 
        require_auth(chost);

    account_index accounts(_me, _me.value);
    auto main_host = host;
    auto account = accounts.find(main_host.value);

    eosio::check(account != accounts.end(), "Account is not upgraded to Host. Please update your account and try again.");
    
    auto root_symbol = account -> get_root_symbol();
    auto chosts = account->chosts;

    if (account->non_active_chost == true) {
        main_host = chosts.back();
        eosio::check(account->ahost != main_host, "Protocol is already active and cannot be changed now.");
        eosio::check(main_host == chost, "Wrong last non-active child host");
        // spiral_index last_params (_me, (account->ahost).value);
        // auto lp = last_params.find(0);
        // eosio::check(lp->size_of_pool <= size_of_pool, "Size of pool cannot be decreased in child host");
    } else {
        eosio::check(account->activated == false, "Protocol is already active and cannot be changed now.");
    }

    rate_index rates(_me, main_host.value);
    spiral_index spiral(_me, main_host.value);
    

    eosio::check((overlap > 10000) && (overlap <= 100000000), "Overlap factor must be greater then 10000 (1.0000) and less then 20000 (2.0000))");
    eosio::check(profit_growth <= 100000000, "Profit growth factor must be greater or equal 0 (0.0000) and less then 1000000 (100.0000)).");
    eosio::check((loss_percent > 0 ) && ( loss_percent <= 1000000), "Loss Percent must be greater then 0 (0%) and less or equal 1000000 (100%)");
    eosio::check((base_rate >= 100) && (base_rate < 1000000000), "Base Rate must be greater or equal 100 and less then 1e9");
    eosio::check((size_of_pool >= 10) && (size_of_pool <= 1000000000000), "Size of Pool must be greater or equal 10 and less then 1e9");
    eosio::check((pool_limit >= 3) && (pool_limit < 1000), "Pool Count must be greater or equal 4 and less or equal 1000");
    eosio::check((pool_timeout >= 1) && (pool_timeout <= 2000000000),"Pool Timeout must be greater or equal then 1 sec and less then 7884000 sec");
    eosio::check(compensator_percent <= loss_percent, "Compensator Percent must be greater then 0 (0%) and less or equal 10000 (100%)");
    
    auto sp = spiral.find(0);
    auto rate = rates.find(0);
    
    if (((sp != spiral.end()) && rate != rates.end()) && ((account -> activated == false) || (account->ahost != main_host))){
        spiral.erase(sp);
        auto it = rates.begin();
        while (it != rates.end()) {
            it = rates.erase(it);
        };
    };

    spiral.emplace(_me, [&](auto &s) {
        s.overlap = overlap;
        s.quants_precision = quants_precision;
        s.size_of_pool = size_of_pool;
        s.profit_growth = profit_growth;
        s.base_rate = base_rate;
        s.loss_percent = loss_percent;
        s.pool_limit = pool_limit;
        s.pool_timeout = pool_timeout;
        s.priority_seconds = priority_seconds;
    });

    
    
    double buy_rate[pool_limit+1];
    double sell_rate[pool_limit+1];
    double convert_rate[pool_limit+1];
    double delta[pool_limit+1];
    double client_income[pool_limit+1];
    double pool_cost[pool_limit+1];
    double total_in_box[pool_limit+1];
    double payment_to_wins[pool_limit+1];
    double payment_to_loss[pool_limit+1];
    double system_income[pool_limit+1];
    double live_balance_for_sale[pool_limit+1];
    


    /**
     *     Математическое ядро алгоритма курса двойной спирали.
     */

    for (auto i=0; i < pool_limit; i++ ){
         if (i == 0){
            buy_rate[i] = base_rate;
            sell_rate[i] = base_rate;

            delta[i] = 0;
            delta[i + 1] = 0;

            client_income[i] = 0;
            client_income[i + 1] = 0;
            
            pool_cost[i] = size_of_pool * buy_rate[i];
            total_in_box[i] = pool_cost[i];
            
            payment_to_wins[i] = 0;
            payment_to_loss[i] = 0;
            system_income[i] = 0;
            live_balance_for_sale[i] = total_in_box[i];

        } else if (i > 0){ 
            sell_rate[i + 1] = buy_rate[i-1] * overlap / ONE_PERCENT;
            
            client_income[i+1] = uint64_t(sell_rate[i+1]) - uint64_t(buy_rate[i-1]);
            delta[i+1] = i > 1 ? uint64_t(client_income[i+1] * profit_growth / ONE_PERCENT + delta[i]) : client_income[i+1];
            buy_rate[i] = uint64_t(sell_rate[i+1] + delta[i+1]);
            pool_cost[i] = uint64_t(size_of_pool * buy_rate[i]);
            payment_to_wins[i] = i > 1 ? uint64_t(size_of_pool * sell_rate[i]) : 0;
            live_balance_for_sale[i] = uint64_t(pool_cost[i] - payment_to_wins[i]);
            
            if (i == 1){
                sell_rate[i] = buy_rate[i];
                payment_to_loss[i] = 0;
            
            } else {
                payment_to_loss[i] = uint64_t(pool_cost[i - 1] * (HUNDR_PERCENT - loss_percent) / HUNDR_PERCENT);
            
                eosio::check(buy_rate[i] > buy_rate[i-1], "Buy rate should only growth");
                
                //необходимо переработать решение дифференциального уравнения реферальных поступлений и ассерт может быть снят
                eosio::check(payment_to_loss[i] >= payment_to_loss[i-1], "Payment to losses should not growth");
                
            }
            
            system_income[i] = i > 1 ? total_in_box[i-1] - payment_to_wins[i] - payment_to_loss[i] : 0; 
            total_in_box[i] = i > 1 ? total_in_box[i-1] + live_balance_for_sale[i] : total_in_box[i-1] + pool_cost[i];
            

            // Используем по причине необходимости переработки решения дифференциального уравнения системного дохода. Ограничение значительно уменьшает количество возможных конфигураций. 
            eosio::check(system_income[i] >= system_income[i-1], "System income should not decrease");

            /**
             * Проверка бизнес-модели на положительный баланс. 
             * Остатка на балансе в любой момент должно быть достаточно для выплат всем проигравшим и всем победителям. 
             * Если это не так - протокол не позволит себя создать. 
             */

            bool positive = total_in_box[i-1] - payment_to_wins[i] <= payment_to_loss[i] ? false : true;
            

            eosio::check(positive, "The financial model is negative. Change the 2Helix params.");


            if (i > 2)
                eosio::check((client_income[i-1] > 0), "Try to increase Overlap, Size of Pool or Base Rate");      
        } 
    };

    /**
     * Установка таблиц курсов в область памяти хоста
     */

    for (auto i = 0; i < pool_limit; i++){
        rates.emplace(_me, [&](auto &r){
            r.pool_id = i;
            r.buy_rate = buy_rate[i];
            r.sell_rate = sell_rate[i];
            r.client_income = asset(client_income[i], root_symbol);
            r.delta = asset(delta[i], root_symbol);
            r.quant_buy_rate = asset(buy_rate[i], root_symbol);
            r.quant_sell_rate = asset(sell_rate[i], root_symbol);
            r.quant_convert_rate = asset(0, root_symbol);
            r.pool_cost = asset(pool_cost[i], root_symbol);
            r.payment_to_wins = asset(payment_to_wins[i], root_symbol);
            r.payment_to_loss = asset(payment_to_loss[i], root_symbol);
            r.total_in_box = asset(total_in_box[i], root_symbol);
            r.system_income = asset(system_income[i], root_symbol);
            r.live_balance_for_sale = asset(live_balance_for_sale[i], root_symbol);
            r.live_balance_for_convert = asset(0, root_symbol);
            // r.total_on_convert = asset(0, root_symbol);
        });
    }

    accounts.modify(account, _me, [&](auto &a){
        a.parameters_setted = true;
        a.sale_shift = base_rate;
    });

}


void unicore::deposit ( eosio::name username, eosio::name host, eosio::asset amount, eosio::name code, std::string message ){
    
    eosio::check( amount.is_valid(), "Rejected. Invalid quantity" );

    partners2_index users(_partners,_partners.value);
    print("on deposit");
    pool_index pools(_me, host.value);

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    
    eosio::name main_host = acc->get_ahost();

    eosio::check(acc->root_token_contract == code, "Wrong token contract for this host");
    
    rate_index rates(_me, main_host.value);

    spiral_index spiral(_me, main_host.value);
    auto sp = spiral.find(0);

    auto root_symbol = acc->get_root_symbol();

    eosio::check ( amount.symbol == root_symbol, "Rejected. Invalid symbol for this contract.");
    eosio::check(acc != accounts.end(), "Rejected. Host is not founded.");
    eosio::check(acc -> activated == true, "Rejected. Protocol is not active");

    auto pool = pools.find( acc -> current_pool_id );

    eosio::check(pool -> remain_quants <= pool->total_quants, "System Error");
    hoststat_index hoststat(_me, host.value);
    auto ustat = hoststat.find(username.value);

    uint64_t max_deposit = unicore::getcondition(host, "maxdeposit");

    if (max_deposit > 0 ) {
        if (ustat == hoststat.end()) {
            eosio::check(amount.amount <= max_deposit, "Вы достигли предела вкладов в этой кассе.");
        } else {
            eosio::check(amount.amount + ustat -> blocked_now.amount <= max_deposit , "Вы достигли предела вкладов в этой кассе.");
        }
    }
    
    if (pool -> pool_num > 2){
        eosio::check ( pool -> pool_expired_at >= eosio::time_point_sec(eosio::current_time_point().sec_since_epoch()), "Pool is Expired");
    } else {
        eosio::check( pool -> pool_started_at <= eosio::time_point_sec(eosio::current_time_point().sec_since_epoch()), "Pool is not started yet");
    };

    auto rate = rates.find( pool-> pool_num - 1 );
    
    uint128_t dquants = uint128_t(sp -> quants_precision * (uint128_t)amount.amount / (uint128_t)rate -> buy_rate);
    uint64_t quants = dquants;
    eosio::check(pool -> remain_quants >= quants, "Not enought Quants in target pool");
    unicore::fill_pool(username, host, quants, amount, acc -> current_pool_id);
    unicore::add_coredhistory(host, username, acc -> current_pool_id, amount, "deposit", message);
    unicore::add_user_stat("deposit"_n, username, acc->root_token_contract, amount, asset(0, amount.symbol));
    unicore::add_host_stat("deposit"_n, username, host, amount);
    unicore::add_host_stat2("deposit"_n, username, host, amount);
    unicore::add_core_stat("deposit"_n, host, amount);
        
    unicore::refresh_state(host);
    
};

/**
 * @brief      Приватный метод обновления истории ядра
 * 
 */

void unicore::add_coredhistory(eosio::name host, eosio::name username, uint64_t pool_id, eosio::asset amount, std::string action, std::string message){
    coredhistory_index coredhistory(_me, host.value);
    auto coredhist_start = coredhistory.begin();
    auto coredhist_end = coredhistory.rbegin();
    eosio::name payer = action == "deposit" || "convert" ? _me : username;

    pool_index pools(_me, host.value);
    auto pool = pools.find(pool_id);

    coredhistory.emplace(payer, [&](auto &ch){
        ch.id = coredhistory.available_primary_key();
        ch.username = username;
        ch.pool_id = pool_id;
        ch.pool_num = pool -> pool_num;
        ch.cycle_num = pool -> cycle_num;
        ch.color = pool -> color;
        ch.amount = amount;
        ch.action = action;
        ch.message = message;
    });

    if (coredhist_start != coredhistory.end())
        if (coredhist_start->id != coredhist_end->id){ 
            uint64_t distance = coredhist_end -> id - coredhist_start -> id;
            if (distance > MAX_CORE_HISTORY_LENGTH) { 
                coredhistory.erase(coredhist_start);
            };
        }
}


void unicore::refresh_state(eosio::name host){

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    auto root_symbol = acc->get_root_symbol();

    eosio::check(acc != accounts.end(), "Host is not found");
    
    if (acc -> activated == true){

        eosio::name main_host = acc->get_ahost();

        pool_index pools(_me, host.value);
        spiral_index spiral(_me, main_host.value);
        auto sp = spiral.find(0);
        auto pool = pools.find(acc -> current_pool_id);
        // Если пул истек, или доступных пулов больше нет, или оставшихся лепт больше нет, то новый цикл
        
        if ((pool -> pool_expired_at < eosio::time_point_sec(eosio::current_time_point().sec_since_epoch()) || \
            ((pool -> pool_num + 1 > sp-> pool_limit) && (pool -> remain_quants < sp -> quants_precision)))){
            start_new_cycle(host);
        } else if ((pool -> remain_quants < sp -> quants_precision)) {
        // Если просто нет лепт - новый пул. 
        // На случай, если приоритетные пулы полностью заполнены с остатком менее 1 Quant. 
            next_pool(host);
            
        } 

        powerstat_index powerstat(_me, host.value);
        auto pstat = powerstat.find(0);

        if (pstat != powerstat.end()) {
            //calculate id for current distribution
            uint64_t now_secs = eosio::current_time_point().sec_since_epoch();
            uint64_t first_window_start_secs = pstat -> window_open_at.sec_since_epoch();
            uint64_t cycle = (now_secs - first_window_start_secs) / _WINDOW_SECS;

            auto current_pstat = powerstat.find(cycle);
            
            if (current_pstat == powerstat.end()) {

                powerstat.emplace(_me, [&](auto &ps) {
                    ps.id = cycle;
                    ps.window_open_at = eosio::time_point_sec(first_window_start_secs + _WINDOW_SECS * cycle);
                    ps.window_closed_at = eosio::time_point_sec(first_window_start_secs + _WINDOW_SECS * (cycle + 1));
                    ps.liquid_power = acc -> total_shares;
                    ps.total_available = asset(0, root_symbol);
                    ps.total_remain = asset(0, root_symbol);
                    ps.total_distributed = asset(0, root_symbol);
                    ps.total_partners_distributed = asset(0, root_symbol);
                    ps.total_partners_available = asset(0, root_symbol);


                    action(
                        permission_level{ _me, "active"_n },
                        "eosio"_n, "pushupdate"_n,
                        std::make_tuple( host , acc -> current_cycle_num, acc -> current_pool_id, pool -> pool_expired_at, ps.window_closed_at) 
                    ).send();
                });

            } 

        } else {
            //emplace first window
            powerstat.emplace(_me, [&](auto &ps){
                ps.id = 0;
                ps.window_open_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
                ps.window_closed_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + _WINDOW_SECS);
                ps.liquid_power = acc -> total_shares;
                ps.total_available = asset(0, root_symbol);
                ps.total_remain = asset(0, root_symbol);
                ps.total_distributed = asset(0, root_symbol);
                ps.total_partners_distributed = asset(0, root_symbol);
                ps.total_partners_available = asset(0, root_symbol);

                action(
                    permission_level{ _me, "active"_n },
                    "eosio"_n, "pushupdate"_n,
                    std::make_tuple( host , acc -> current_cycle_num, acc -> current_pool_id, pool -> pool_expired_at, ps.window_closed_at) 
                ).send();
            });
        }
    }
   
}
/**
 * @brief      Публичный метод обновления состояния
 * Проверяет пул на истечение во времени или завершение целого количества ядерных Юнитов. Запускает новый цикл или добавляет новый пул. 
 * 
 * //TODO устранить избыточность
 *
 * @param[in]  host  The host
 */

[[eosio::action]] void unicore::refreshst(eosio::name username, eosio::name host){
    unicore::refresh_state(host);
};


/**
 * @brief      Публичный метод продажи баланса
 * 
 * 
 * 
 *
 * @param[in]  host  The host
 */

[[eosio::action]] void unicore::sellbalance(eosio::name host, eosio::name username, uint64_t balance_id){
    unicore::refresh_state(host);

    eosio::check(has_auth(username) || has_auth(_self), "missing required authority");

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "host is not found");
    
    auto root_symbol = acc->get_root_symbol();
    balance_index balance(_me, host.value);
    
    auto bal = balance.find(balance_id);
    eosio::check(bal != balance.end(), "Balance is not found");
    auto main_host = bal -> get_ahost();
    
    
    pool_index pools(_me, host.value);
    rate_index rates(_me, main_host.value);
    cycle_index cycles(_me, host.value);
    spiral_index spiral(_me, main_host.value);
    
    auto sp = spiral.find(0);
    
    auto pool = pools.find(bal->global_pool_id);
    
    auto cycle = cycles.find(pool -> cycle_num - 1);
    
    eosio::check(bal -> last_recalculated_win_pool_id == cycle->finish_at_global_pool_id, "Cannot sell not refreshed balance. Refresh Balance first and try again.");
    eosio::check(bal -> win == false, "This balance does not require a delayed sell");
    eosio::check(bal -> global_pool_id < bal -> last_recalculated_win_pool_id, "This balance does not require a delayed sell");
    eosio::check(acc -> current_pool_num > 2, "Only balances from pool_num > 2 can be solded");
    balance.modify(bal, _me, [&](auto &b){
        b.status = "onsell"_n;
    });

};



/**
 * @brief      Публичный метод продажи баланса
 * 
 * 
 * 
 *
 * @param[in]  host  The host
 */

[[eosio::action]] void unicore::cancelsellba(eosio::name host, eosio::name username, uint64_t balance_id){
    unicore::refresh_state(host);

    eosio::check(has_auth(username) || has_auth(_self), "missing required authority");

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "host is not found");
    // eosio::name main_host = acc->get_ahost();
    
    auto root_symbol = acc->get_root_symbol();
    balance_index balance(_me, host.value);
    
    auto bal = balance.find(balance_id);
    eosio::check(bal != balance.end(), "Balance is not found");
    auto main_host = bal -> get_ahost();
    
    eosio::check(bal -> status != "process"_n, "Balance not in the sale");

    if (bal -> solded_for.amount > 0) {
        action(
            permission_level{ _me, "active"_n },
            acc->root_token_contract, "transfer"_n,
            std::make_tuple( _me, username, bal -> solded_for, std::string("User Solded Withdraw")) 
        ).send();
    }

    if (bal -> compensator_amount.amount > 0) {

        balance.modify(bal, _me, [&](auto &b){
            b.status = "process"_n;
            // b.global_pool_id = bal -> last_recalculated_win_pool_id;
            b.purchase_amount = bal -> compensator_amount;
            b.solded_for = asset(0, root_symbol);
        });

    } else balance.erase(bal);
    
};


void unicore::buybalance(eosio::name buyer, eosio::name host, uint64_t balance_id, eosio::asset amount, eosio::name contract){
    require_auth(buyer);

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "host is not found");
    eosio::name main_host = acc->get_ahost();
    
    cycle_index cycles(_me, host.value);
    pool_index pools(_me, host.value);
    rate_index rates(_me, main_host.value);
    spiral_index spiral(_me, main_host.value);

    auto sp = spiral.find(0);
    

    eosio::check(contract == acc -> root_token_contract, "Wrong token contract for this host");

    auto root_symbol = acc->get_root_symbol();
    balance_index balance(_me, host.value);
    
    auto bal = balance.find(balance_id);
    eosio::check(bal != balance.end(), "Balance is not found");
    
    eosio::check(bal -> status == "onsell"_n, "This balance not on the sale");

    
    auto pool = pools.find(bal -> last_recalculated_win_pool_id - 1);
    auto cycle = cycles.find(pool -> cycle_num - 1);
    
    // eosio::check(bal -> last_recalculated_win_pool_id == cycle->finish_at_global_pool_id, "Cannot buy not refreshed balance. Refresh Balance first and try again.");

    auto user_rate = rates.find(pool -> pool_num - 1);
    auto next_user_win_rate = rates.find(pool -> pool_num);

    auto current_rate = rates.find(pool -> pool_num);

    eosio::check(amount <= bal -> compensator_amount, "Amount for sell should be less or equal compensator amount");
    
    std::vector<eosio::asset> forecasts;
    uint64_t buyer_quants = 0;
    uint64_t buyer_next_quants = 0;
    uint64_t buyer_if_convert = 0 ;
    eosio::asset old_compensator_amount = bal -> compensator_amount;

     balance.modify(bal, _me, [&](auto &b) {
        
        buyer_quants = amount * bal -> quants_for_sale / bal -> compensator_amount;

        b.quants_for_sale = bal -> quants_for_sale - buyer_quants;
        
        buyer_next_quants = amount * bal -> next_quants_for_sale / bal -> compensator_amount;

        b.next_quants_for_sale = bal -> next_quants_for_sale - buyer_next_quants;
        
        b.compensator_amount = bal -> compensator_amount - amount;

        b.status = b.compensator_amount.amount == 0 ? "solded"_n : "onsell"_n;
        

        b.solded_for = bal-> solded_for + amount;
        b.forecasts = forecasts;

        eosio::asset old_if_convert = bal -> if_convert;
        buyer_if_convert = amount.amount * old_if_convert.amount / old_compensator_amount.amount;
        print("LOOK HERE: ");
        print("old_if_convert: ", old_if_convert);
        print("amount.amount: ", amount.amount);
        print("bal -> compensator_amount.amount: ", old_compensator_amount.amount);
        print("buyer_if_convert: ", buyer_if_convert);
        b.if_convert = old_if_convert - asset(buyer_if_convert, _POWER);
        print("b.if_convert: ", b.if_convert);
        eosio::asset if_convert_diff = old_if_convert - b.if_convert;
        print("if_convert_diff: ", if_convert_diff);
        
        b.if_convert_to_power = b.if_convert;
        b.start_convert_amount = asset(0, _POWER);

        double convert_percent = (double)b.if_convert.amount / (double)sp->quants_precision / ((double)user_rate -> total_in_box.amount - (double)pool -> remain.amount) * (double)HUNDR_PERCENT;
        b.convert_percent = uint64_t(ONE_PERCENT * convert_percent);


        double root_percent = (double)b.compensator_amount.amount  / (double)b.purchase_amount.amount * (double)HUNDR_PERCENT - (double)HUNDR_PERCENT; 
        b.root_percent = uint64_t(root_percent);



        action(
            permission_level{ _me, "active"_n },
            _me, "emitpower"_n,
            std::make_tuple( host , bal -> owner, - if_convert_diff.amount, true) 
        ).send();
    });



    balance.emplace(_me, [&](auto &b){
        b.id = get_global_id("balances"_n);
        b.owner = buyer;
        b.cycle_num = bal->cycle_num;
        b.pool_num = bal->pool_num;
        b.host = bal -> host;
        b.chost = bal -> chost;
        
        b.global_pool_id = bal -> global_pool_id; 
        
        b.pool_color = bal -> pool_color;
        b.quants_for_sale = buyer_quants;
        b.next_quants_for_sale = buyer_next_quants;
        
        b.purchase_amount = amount;
        b.available = asset(0, root_symbol);
        b.compensator_amount = amount;

        b.if_convert = asset(buyer_if_convert, _POWER);
        b.if_convert_to_power = asset(buyer_if_convert, _POWER);
        b.start_convert_amount = asset(buyer_if_convert, _POWER);
        
        double convert_percent = (double)b.if_convert.amount / (double)sp->quants_precision / ((double)user_rate -> total_in_box.amount - (double)pool -> remain.amount) * (double)HUNDR_PERCENT;
        b.convert_percent = uint64_t(ONE_PERCENT * convert_percent);


        double root_percent = (double)b.compensator_amount.amount  / (double)b.purchase_amount.amount * (double)HUNDR_PERCENT - (double)HUNDR_PERCENT; 
        b.root_percent = uint64_t(root_percent);

        b.last_recalculated_win_pool_id = bal -> last_recalculated_win_pool_id;
        b.forecasts = forecasts;

        b.ref_amount = asset(0, root_symbol);
        b.dac_amount = asset(0, root_symbol);
        b.cfund_amount = asset(0, root_symbol);
        b.hfund_amount = asset(0, root_symbol);
        b.sys_amount = asset(0, root_symbol);
        b.solded_for = asset(0, root_symbol);
        
        action(
            permission_level{ _me, "active"_n },
            _me, "emitpower"_n,
            std::make_tuple( host , buyer, b.if_convert.amount, true) 
        ).send();

        print("BALANCE_ID: ", b.id, ";"); //DO NOT DELETE THIS
    });

};

/**
 * @brief      Внутренний метод заполнения пула.
 * Вызывается в момент совершения депозита пользователем или на приоритетном входе. Создает баланс пользователю и уменьшает количество квантов в пуле. 
 *
 * @param[in]  username        The username
 * @param[in]  host            The host
 * @param[in]  quants          The quants
 * @param[in]  amount          The amount
 * @param[in]  filled_pool_id  The filled pool identifier
 */


void unicore::fill_pool(eosio::name username, eosio::name host, uint64_t quants, eosio::asset amount, uint64_t filled_pool_id){
    std::vector<eosio::asset> forecasts;
    account_index accounts(_me, _me.value);
    
    auto acc = accounts.find(host.value);
    auto main_host = acc -> get_ahost();

    cycle_index cycles(_me, host.value);
    pool_index pools(_me, host.value);
    rate_index rates(_me, main_host.value);
    spiral_index spiral(_me, main_host.value);

    auto sp = spiral.find(0);
    balance_index balance(_me, host.value);
    
    auto root_symbol = acc->get_root_symbol();
    auto pool = pools.find(filled_pool_id);

    eosio::check(pool != pools.end(), "Cant FIND");
    


    auto rate = rates.find(pool -> pool_num + 1);
    auto ratep1 = rates.find(pool -> pool_num);
    auto ratem1 = rates.find(pool -> pool_num - 1);
    auto ratem2 = rates.find(pool -> pool_num - 2);

    uint64_t next_quants_for_sale;
    uint64_t remain_quants;

    if (rate == rates.end())
        next_quants_for_sale = quants;    
    else 
        next_quants_for_sale = quants * rate->sell_rate / rate->buy_rate;
    
    uint64_t b_id = 0;

    eosio::check(quants <= pool -> remain_quants, "Not enought quants in target pool. Deposit is prevented for core safety");

    remain_quants = pool -> remain_quants - quants;
    
    //TODO REFACTOR IT!
    if (acc->current_pool_num == 1)
        forecasts = unicore::calculate_forecast(username, host, quants, pool -> pool_num - 1, amount, true, false);
    else if (acc->current_pool_num == 2)
        forecasts = unicore::calculate_forecast(username, host, quants, pool -> pool_num - 1, amount, true, true);
    else
        forecasts = unicore::calculate_forecast(username, host, quants, pool -> pool_num - 1, amount, true, true);


    pools.modify(pool, _me, [&](auto &p){
        p.remain_quants = remain_quants;

        uint128_t filled = (pool->total_quants / sp -> quants_precision - p.remain_quants / sp -> quants_precision) * pool->quant_cost.amount;

        p.filled = asset(uint64_t(filled), root_symbol);
        p.remain = p.pool_cost - p.filled;
        p.filled_percent = (pool->total_quants - p.remain_quants)  * HUNDR_PERCENT / pool->total_quants;
        
    });

    uint64_t total_quants = sp->size_of_pool * sp -> quants_precision;

    
    unicore::change_bw_trade_graph(host, filled_pool_id, pool->cycle_num, pool->pool_num, ratem1->buy_rate, ratep1->buy_rate, total_quants, remain_quants, pool->color);
    
    // double convert_rate = (double)sp -> base_rate; //(double)ratem1 -> buy_rate;// / (double)sp -> base_rate;
    double convert_rate = (double)ratem1 -> buy_rate;// / (double)sp -> base_rate;
    
    if (acc -> sale_shift == 0 || acc -> sale_shift < ratem1 -> buy_rate) {
        accounts.modify(acc, _me, [&](auto &a) {
            // print("CORRECT SALE_SHIFT ON FILL POOL: ", a.sale_shift);
            a.sale_shift = ratem1 -> buy_rate;
        });
    } else {
    
        convert_rate = acc -> sale_shift;
    
    }
    double if_convert_amount = double(amount.amount) * (double)sp->quants_precision / double(convert_rate);// / double(sp -> base_rate);
    
    
    spiral2_index spiral2(_me, host.value);
    auto sp2 = spiral2.find(0);
    
    uint64_t balance_id = get_global_id("balances"_n);
    gpercents_index gpercents(_me, _me.value);
    auto syspercent = gpercents.find("system"_n.value);

    eosio::asset asset_ref_amount = asset(0, root_symbol);

    balance.emplace(_me, [&](auto &b){
        b.id = balance_id;
        b.owner = username;
        b.cycle_num = pool->cycle_num;
        b.pool_num = pool->pool_num;
        b.host = host;
        b.chost = main_host;
        b.global_pool_id = filled_pool_id; 
        b.pool_color = pool -> color;
        b.quants_for_sale = quants;
        b.next_quants_for_sale = next_quants_for_sale;
        b.purchase_amount = amount;
        b.available = amount;
        b.compensator_amount = amount;
        b.solded_for = asset(0, root_symbol);

        b.if_convert = asset(uint64_t(if_convert_amount), _POWER);
        b.if_convert_to_power = b.if_convert;
        b.start_convert_amount = b.if_convert;
        
        double convert_percent = (double)b.if_convert.amount / (double)sp->quants_precision / ((double)ratem1 -> total_in_box.amount - (double)pool -> remain.amount) * (double)HUNDR_PERCENT;
        b.convert_percent = uint64_t(ONE_PERCENT * convert_percent);

        double root_percent = (double)b.compensator_amount.amount  / (double)b.purchase_amount.amount * (double)HUNDR_PERCENT - (double)HUNDR_PERCENT; 
        b.root_percent = uint64_t(root_percent);

        b.last_recalculated_win_pool_id = pool -> pool_num == 1 ? filled_pool_id + 1 : filled_pool_id;
        b.forecasts = forecasts;
        b.ref_amount = asset_ref_amount;
        b.dac_amount = asset(0, root_symbol);
        b.cfund_amount = asset(0, root_symbol);
        b.hfund_amount = asset(0, root_symbol);
        b.sys_amount = asset(0, root_symbol);

        action(
            permission_level{ _me, "active"_n },
            _me, "emitpower"_n,
            std::make_tuple( host , username, b.if_convert.amount, false) 
        ).send();

    });
    
    print("BALANCE_ID: ", balance_id, ";"); //DO NOT DELETE THIS
}

/**
 * @brief      Публичный метод обновления баланса
 * Пересчет баланса каждого пользователя происходит по его собственному действию. Обновление баланса приводит к пересчету доступной суммы для вывода. 
 *
 * @param[in]  op    The operation
 */

[[eosio::action]] void unicore::refreshbal(eosio::name username, eosio::name host, uint64_t balance_id, uint64_t partrefresh){
    eosio::check(has_auth(username) || has_auth("refresher"_n) || has_auth("eosio"_n), "missing required authority");
    
    eosio::name payer;

    if (has_auth(username)) {
        payer = username;
    } else if (has_auth("refresher"_n))
        payer = "refresher"_n;
    else payer = "eosio"_n;

    balance_index balance(_me, host.value);
    auto bal = balance.find(balance_id);
    eosio::check(bal != balance.end(), "Balance is not exist or already withdrawed");
    
    auto chost = bal -> get_ahost();
    auto parent_host = bal -> host;
    account_index accounts(_me, _me.value);
    
    auto acc = accounts.find(parent_host.value);

    auto root_symbol = acc->get_root_symbol();
    
    unicore::refresh_state(parent_host);
    
    cycle_index cycles(_me, parent_host.value);
    rate_index rates(_me, chost.value);
    pool_index pools(_me, parent_host.value);
    spiral_index spiral(_me, chost.value);

    auto sp = spiral.find(0);
    
    spiral2_index spiral2(_me, chost.value);

    auto sp2 = spiral2.find(0);
    

    auto pool_start = pools.find(bal -> global_pool_id);
    auto cycle = cycles.find(pool_start -> cycle_num - 1);
    auto next_cycle = cycles.find(pool_start -> cycle_num);
    auto pools_in_cycle = cycle -> finish_at_global_pool_id - cycle -> start_at_global_pool_id + 1;
    
    auto last_pool = pools.find(cycle -> finish_at_global_pool_id );
    auto has_new_cycle = false;

    if (next_cycle != cycles.end())
        has_new_cycle = true;

    uint64_t ceiling;

    if (( partrefresh != 0 )&&(bal -> last_recalculated_win_pool_id + partrefresh < cycle -> finish_at_global_pool_id)){
        ceiling = bal -> last_recalculated_win_pool_id + partrefresh;

    } else {
        ceiling = cycle -> finish_at_global_pool_id;
    }

    if (bal -> status == "process"_n && bal -> last_recalculated_win_pool_id <= cycle -> finish_at_global_pool_id)
        for (auto i = bal -> last_recalculated_win_pool_id + 1;  i <= ceiling; i++){
            std::vector<eosio::asset> forecasts;
            
            auto look_pool = pools.find(i);
            auto purchase_amount = bal-> purchase_amount;
            auto rate = rates.find(look_pool -> pool_num - 1);
                
            
            if (((acc -> current_pool_num == pool_start -> pool_num ) && (acc -> current_cycle_num == pool_start -> cycle_num)) || \
                ((pool_start -> pool_num < 3) && (pools_in_cycle < 3)) || (has_new_cycle && (pool_start->pool_num == last_pool -> pool_num)))

            { //NOMINAL
                eosio::asset old_if_convert = bal -> if_convert;

                balance.modify(bal, payer, [&](auto &b){
                    b.last_recalculated_win_pool_id = i;
                    b.available = purchase_amount;
                    double double_if_convert = (double)purchase_amount.amount * (double)sp->quants_precision / (double)acc -> sale_shift;// / sp -> base_rate;

                    b.if_convert = asset(uint64_t(double_if_convert), _POWER);
                    b.if_convert_to_power = b.if_convert;

                    // double convert_percent = (double)b.if_convert.amount  * (double)HUNDR_PERCENT / (double)amount.amount - (double)HUNDR_PERCENT;

                    double convert_percent = (double)b.if_convert.amount / (double)sp->quants_precision / ((double)rate -> total_in_box.amount - (double)look_pool -> remain.amount) * (double)HUNDR_PERCENT;
                    b.convert_percent = uint64_t(ONE_PERCENT * convert_percent);

                    print("power emit on nominal: ", old_if_convert.amount - b.if_convert.amount);
                    int64_t emit_power = int64_t(old_if_convert.amount) - int64_t(b.if_convert.amount);

                    action(
                        permission_level{ _me, "active"_n },
                        _me, "emitpower"_n,
                        std::make_tuple( host , username, emit_power, false) 
                    ).send();

                });

            } else {
                eosio::asset available;
                uint64_t new_reduced_quants;
                uint64_t new_quants_for_sale;
                eosio::asset ref_amount = asset(0, root_symbol);
                auto prev_win_rate = rates.find(look_pool -> pool_num - 3);
                auto middle_rate = rates.find(look_pool -> pool_num - 2);

                gpercents_index gpercents(_me, _me.value);
                auto syspercent = gpercents.find("system"_n.value);
                    

                if (pool_start -> color == look_pool -> color) {
                    //WIN
                    if (look_pool -> pool_num - pool_start -> pool_num <= 2) {
                        
                        new_reduced_quants = bal -> quants_for_sale * rate -> sell_rate / rate -> buy_rate;
                        new_quants_for_sale = bal -> quants_for_sale;
                        
                        if (new_reduced_quants == 0)
                            new_reduced_quants = new_quants_for_sale;
                        forecasts = unicore::calculate_forecast(username, parent_host, new_quants_for_sale, look_pool -> pool_num - 3, bal->available, false, false);
                        available = asset(uint64_t((double)new_quants_for_sale * (double)rate -> sell_rate / (double)sp -> quants_precision), root_symbol);                        
                        
                        // forecasts.erase(forecasts.begin());
                    } else {

                        new_quants_for_sale = bal -> quants_for_sale * prev_win_rate -> sell_rate / prev_win_rate -> buy_rate;
                        new_reduced_quants = new_quants_for_sale * rate -> sell_rate / rate -> buy_rate;
                        
                        if (new_reduced_quants == 0)
                            new_reduced_quants = new_quants_for_sale;
                        forecasts = unicore::calculate_forecast(username, parent_host, new_quants_for_sale, look_pool -> pool_num - 3, bal->available, false, false);
                        available = asset(uint64_t((double)new_quants_for_sale * (double)rate -> sell_rate / (double)sp -> quants_precision), root_symbol);                        
                    
                    }

                       
                    /**
                    Для расчетов выплат реферальных вознаграждений необходимо решить дифференциальное уравнение. 
                     */

                    auto start_rate = prev_win_rate;
                    auto finish_rate = rate;
                    
                    uint64_t ref_quants;
                    eosio::asset asset_ref_amount;
                    eosio::asset asset_sys_amount;
                    eosio::asset asset_dac_amount;
                    eosio::asset asset_cfund_amount;
                    eosio::asset asset_hfund_amount;

                    
                    eosio::check(syspercent != gpercents.end(), "Contract is not active");
                    
                    if (look_pool -> pool_num - bal -> pool_num < 2){
                        ref_quants = bal->quants_for_sale;
                    } else {
                        ref_quants = new_quants_for_sale;
                    };
                    
                    // ref_quants = new_reduced_quants; //new_quants_for_sale;

                    
                    // Здесь необходимо взять в расчеты только те границы, при которых системный доход рос, тогда, в setparams можно снять ограничение на это. 
                    if ((middle_rate -> system_income >= start_rate -> system_income) && (finish_rate -> system_income >= middle_rate -> system_income)){
                        
                        print(" start_rate: ", start_rate -> system_income);
                        print(" middle_rate: ", middle_rate -> system_income);
                        
                        // print(" start_rate: ", start_rate -> system_income);
                        
                        eosio::asset incr_amount1 = middle_rate -> system_income - start_rate -> system_income;
                        eosio::asset incr_amount2 = finish_rate -> system_income - middle_rate -> system_income;
                        
                        uint64_t total = incr_amount2.amount / 2;// + incr_amount2.amount; //incr_amount2.amount;// - incr_amount1.amount // //incr_amount2.amount - incr_amount1.amount;
                        print(" system income for ref calc: ", incr_amount1);
                        print(" total ref: ", asset(total, root_symbol));

                        double total_ref =   (total * (double)ref_quants / (double)sp -> quants_precision * (double)(acc->referral_percent)) / ((double)HUNDR_PERCENT * (double)sp->size_of_pool);
                        double total_ref_min_sys = total_ref * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
                        double total_ref_sys = total_ref * syspercent -> value / HUNDR_PERCENT;


                        double total_dac =   (total * (double)ref_quants / (double)sp -> quants_precision * (double)(acc->dacs_percent))     / ((double)HUNDR_PERCENT * (double)sp->size_of_pool);
                        double total_dac_min_sys = total_dac * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
                        double total_dac_sys = total_dac * syspercent -> value / HUNDR_PERCENT;
                        
                        double total_cfund = (total * (double)ref_quants / (double)sp -> quants_precision * (double)(acc->cfund_percent))    / ((double)HUNDR_PERCENT * (double)sp->size_of_pool);
                        double total_cfund_min_sys = total_cfund * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
                        double total_cfund_sys = total_cfund * syspercent -> value / HUNDR_PERCENT;
                                                
                        double total_hfund = (total * (double)ref_quants / (double)sp -> quants_precision * (double)(acc->hfund_percent))    / ((double)HUNDR_PERCENT * (double)sp->size_of_pool);
                        double total_hfund_min_sys = total_hfund * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
                        double total_hfund_sys = total_hfund * syspercent -> value / HUNDR_PERCENT;
                                                


                        double total_sys = total_ref_sys + total_hfund_sys + total_dac_sys + total_cfund_sys;

                        asset_ref_amount = asset((uint64_t)total_ref_min_sys, root_symbol);
                        asset_dac_amount = asset((uint64_t)total_dac_min_sys, root_symbol);
                        asset_sys_amount = asset((uint64_t)total_sys, root_symbol);
                        asset_cfund_amount = asset((uint64_t)total_cfund_min_sys, root_symbol); 
                        asset_hfund_amount = asset((uint64_t)total_hfund_min_sys, root_symbol);
                         
                        } else {
                            asset_ref_amount = asset(0, root_symbol);
                            asset_dac_amount = asset(0, root_symbol);
                            asset_sys_amount = asset(0, root_symbol);
                            asset_cfund_amount = asset(0, root_symbol);
                            asset_hfund_amount = asset(0, root_symbol);
                        }
                        
                        uint64_t old_convert_amount = bal -> if_convert.amount;
                        
                        

                        print(" asset_ref_amount WIN: ", asset_ref_amount);
                        print(" old bal ref_amount: ", bal -> ref_amount);

                        if ((asset_ref_amount).amount > 0) {
                            unicore::spread_to_refs(host, username, bal -> ref_amount + asset_ref_amount, available, acc -> root_token_contract);
                            sincome_index sincome(_me, host.value);
                            auto sinc = sincome.find(acc -> current_pool_id);

                            sincome.modify(sinc, _me, [&](auto &s) {
                                s.total += bal -> ref_amount + asset_ref_amount;
                                s.paid_to_refs += bal -> ref_amount + asset_ref_amount;
                            });
                        }

                        balance.modify(bal, payer, [&](auto &b){
                            b.last_recalculated_win_pool_id = i;
                            b.quants_for_sale = new_quants_for_sale;
                            b.next_quants_for_sale = new_reduced_quants;
                            b.available = available;
                            b.compensator_amount = available;
                            b.win = true;
                            b.forecasts = forecasts;
                            double root_percent = (double)b.compensator_amount.amount / (double)bal -> purchase_amount.amount * (double)HUNDR_PERCENT  - (double)HUNDR_PERCENT; 
                            b.root_percent = (uint64_t)root_percent;
                            //TODO
                            double double_if_convert = double(available.amount) * (double)sp->quants_precision / double(acc -> sale_shift);// / double(sp -> base_rate);
                            b.if_convert = asset(uint64_t(double_if_convert), _POWER);
                            b.if_convert_to_power = b.if_convert;
                            
                            // b.start_convert_amount = asset(if_convert_amount, _POWER);
                            double convert_percent = (double)b.if_convert.amount / (double)sp->quants_precision / ((double)rate -> total_in_box.amount - (double)look_pool -> remain.amount) * (double)HUNDR_PERCENT;
                            b.convert_percent = uint64_t(ONE_PERCENT * convert_percent);
                            
                            int64_t power_to_emit = int64_t(double(b.if_convert.amount) - double(old_convert_amount));
                            
                            action(
                                permission_level{ _me, "active"_n },
                                _me, "emitpower"_n,
                                std::make_tuple( host , username, power_to_emit, false) 
                            ).send();
  

                        });

                } else {
                    //LOSS
                    
                    double davailable = (double)bal -> available.amount * (double)(HUNDR_PERCENT - sp -> loss_percent) / (double)HUNDR_PERCENT;

                    eosio::asset available = asset(uint64_t(davailable), root_symbol) ;
                    std::vector <eosio::asset> forecasts0 = bal->forecasts;
                    
                    auto distance = look_pool -> pool_num - pool_start -> pool_num;
                    
                    if ( distance > 0 )
                        if (forecasts0.begin() != forecasts0.end())
                            forecasts0.erase(forecasts0.begin());
                    

                    auto middle_rate = rates.find(look_pool -> pool_num - 2);

                    eosio::asset incr_amount2 = (rate -> system_income - middle_rate -> system_income) / 2;

                    uint64_t total = incr_amount2.amount;// + incr_amount2.amount; //incr_amount2.amount;// - incr_amount1.amount // //incr_amount2.amount - incr_amount1.amount;
                    
                    print(" start_rate: ", rate -> system_income);
                    print(" middle_rate: ", middle_rate -> system_income);
                    
                    double ref_quants;
                    
                    print(" bal -> pool_num: ", bal -> pool_num);
                    print(" look_pool -> pool_num: ", look_pool -> pool_num);
                    
                    if (look_pool -> pool_num - bal -> pool_num < 2){
                        ref_quants = bal->quants_for_sale;
                    } else {
                        ref_quants = bal->next_quants_for_sale;
                    };
                    

                    double total_ref =   (total * (double)ref_quants / (double)sp -> quants_precision * (double)(acc->referral_percent)) / ((double)HUNDR_PERCENT * (double)sp->size_of_pool);
                    double total_ref_min_sys = total_ref * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
                    eosio::asset asset_ref_amount = asset((uint64_t)total_ref_min_sys, root_symbol);
                    
                    if (bal -> ref_amount.amount + asset_ref_amount.amount > 0) {
                        unicore::spread_to_refs(host, username, bal -> ref_amount + asset_ref_amount, available, acc -> root_token_contract);
                        
                        sincome_index sincome(_me, host.value);
                        auto sinc = sincome.find(acc -> current_pool_id);

                        sincome.modify(sinc, _me, [&](auto &s) {
                            s.total += bal -> ref_amount + asset_ref_amount;
                            s.paid_to_refs += bal -> ref_amount + asset_ref_amount;
                        });

                    }

                    eosio::asset old_convert_amount = bal -> if_convert;

                    balance.modify(bal, payer, [&](auto &b){
                        b.last_recalculated_win_pool_id = i;
                        b.win = false;
                        b.available = available;
                        b.forecasts = forecasts0;
                        double root_percent = (double)bal->compensator_amount.amount / (double)bal -> purchase_amount.amount * (double)HUNDR_PERCENT  - (double)HUNDR_PERCENT; 
                        b.root_percent = uint64_t(root_percent); 
                    
                        //TODO
                        double double_if_convert = (double)bal->compensator_amount.amount  * (double)sp->quants_precision / (double)acc -> sale_shift;// / (double)sp -> base_rate;
                        b.if_convert = asset(uint64_t(double_if_convert), _POWER);
                        b.if_convert_to_power = asset(b.if_convert.amount, _POWER);
                        double convert_percent = (double)b.if_convert.amount / (double)sp->quants_precision  / ((double)rate -> total_in_box.amount - (double)look_pool -> remain.amount) * (double)HUNDR_PERCENT;
                        b.convert_percent = uint64_t(ONE_PERCENT * convert_percent);
                        b.ref_amount = asset(0, root_symbol);
                        int64_t power_to_emit = (int64_t(b.if_convert.amount) - int64_t(old_convert_amount.amount));
                        
                        action(
                            permission_level{ _me, "active"_n },
                            _me, "emitpower"_n,
                            std::make_tuple( host , username, power_to_emit, false) 
                        ).send();
                    });
                };
            }
        }

}

/**
 * @brief      Метод расчета прогнозов
 * Внутренний метод расчета прогнозов. Внутренний метод расчета прогнозов выплат для последующих 8х бассейнов на основе будущих курсов. Используются только для демонастрации.
 * 
 * 
 * TODO
 * Может дополнительно быть реализован в качестве внешнего метода достоверной проверки прогнозов, который с каждым вызовом производит расчет будущих курсов и расширяет массив с данными по желанию пользователя.
 * Устранить избыток кода.
 * 
 * @param[in]  username  The username
 * @param[in]  host      The host
 * @param[in]  quants    The quants
 * @param[in]  pool_num  The pool number
 * 
 * @return     The forecast.
 */

std::vector <eosio::asset> unicore::calculate_forecast(eosio::name username, eosio::name host, uint64_t quants, uint64_t pool_num, eosio::asset available_amount, bool calculate_first = true, bool calculate_zero = false){
    
    //TODO -> cycle / recursion

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    auto main_host = acc->get_ahost();

    balance_index balance(_me, host.value);
    rate_index rates (_me, main_host.value);
    spiral_index spiral (_me, main_host.value);
    spiral2_index spiral2(_me, main_host.value);

    auto root_symbol = acc->get_root_symbol();
    auto sp = spiral.find(0);
    auto sp2 = spiral2.find(0);

    std::vector<eosio::asset> forcasts;
    double quants_forecast1;
    double quants_forecast2;
    double quants_forecast3;
    double quants_forecast4;
    eosio::asset forecast0;
    eosio::asset forecast1;
    eosio::asset forecast15;
    eosio::asset forecast2;
    eosio::asset forecast25;
    eosio::asset forecast3;
    eosio::asset forecast35;
    eosio::asset forecast4;
    eosio::asset forecast45;
            

    eosio::asset loser_amount = asset(uint64_t((double)available_amount.amount * (double)(HUNDR_PERCENT - sp -> loss_percent) / (double)HUNDR_PERCENT), root_symbol) ;
    
    if (pool_num + 2 < sp->pool_limit  && calculate_zero == true) {
        

        // if (acc->sale_is_enabled == true) {
        //     auto rate0 = rates.find(pool_num + 1);
        //     auto convert_amount = (double)quants * (double)rate0->convert_rate / (double)sp->quants_precision;
        //     if (acc -> sale_mode == "direct"_n) {
        //         forecast0 = asset(available_amount.amount, _POWER);
        //     }
            
            
        // } else {
            forecast0 = loser_amount;
        // }

        forcasts.push_back(forecast0);
          
    }     


    if (pool_num + 3 < sp->pool_limit ) {
        
        auto rate1 = rates.find(pool_num + 2);
        auto amount3 = quants * rate1 -> sell_rate / sp -> quants_precision;
        forecast1 = asset(amount3, root_symbol);
        
        if (calculate_first)
            forcasts.push_back(forecast1);

        quants_forecast1 = quants * rate1 -> sell_rate / rate1 -> buy_rate;

    } 
   
   if (pool_num + 4 < sp->pool_limit){
        auto rate15 = rates.find(pool_num + 3);
        

        // if (acc->sale_is_enabled == true){
        //     double convert_amount = (double)quants * (double)rate15->convert_rate / (double)sp->quants_precision;
        //     if (acc -> sale_mode == "direct"_n){
        //         forecast15 = asset(available_amount.amount, _POWER);
        //     }
        // } else {        
            forecast15 = asset(forecast1.amount * (HUNDR_PERCENT - sp -> loss_percent) / HUNDR_PERCENT, root_symbol) ;
        // }
        
        forcasts.push_back(forecast15);
    }

   
    if (pool_num + 5 < sp->pool_limit){
       
       auto rate2 = rates.find(pool_num + 4);
       double amount5 = (double)quants_forecast1 * (double)rate2 -> sell_rate / (double)sp -> quants_precision;
       forecast2 = asset(amount5, root_symbol);
       quants_forecast2 = quants_forecast1 * rate2 -> sell_rate / rate2 -> buy_rate;
       forcasts.push_back(forecast2);
    
    } 

     if (pool_num + 6 < sp->pool_limit){
        auto rate25 = rates.find(pool_num + 5);
        // if (acc->sale_is_enabled == true){
            
        //     double convert_amount = (double)quants_forecast1 * (double)rate25->convert_rate / (double)sp->quants_precision;       
            
        //     if (acc -> sale_mode == "direct"_n){
        //         forecast25 = asset(available_amount.amount, _POWER);
        //     }

        // } else {
        
            forecast25 = asset(forecast2.amount * (HUNDR_PERCENT - sp -> loss_percent) / HUNDR_PERCENT, root_symbol) ;
        
        // }

        forcasts.push_back(forecast25);
    }



    if (pool_num + 7 < sp->pool_limit){
        auto rate3 = rates.find(pool_num + 6);
        auto amount7 = (double)quants_forecast2 * (double)rate3 -> sell_rate / (double)sp -> quants_precision;
        forecast3 = asset(amount7, root_symbol);
        quants_forecast3 = quants_forecast2 * rate3 -> sell_rate / rate3 -> buy_rate;
       
        forcasts.push_back(forecast3);

    }



    if (pool_num + 8 < sp->pool_limit){
        auto rate35 = rates.find(pool_num + 7);
        // if (acc->sale_is_enabled == true){
            
        //     double convert_amount = (double)quants_forecast2 * (double)rate35->convert_rate  / (double)sp->quants_precision;

        //     if (acc -> sale_mode == "direct"_n){
        //         forecast35 = asset(available_amount.amount, _POWER);
        //     }

            
        
        // } else {
        
            forecast35 = asset(uint64_t((double)forecast3.amount * (double)(HUNDR_PERCENT - sp -> loss_percent) / (double)HUNDR_PERCENT), root_symbol) ;
        
        // }

        forcasts.push_back(forecast35);
    }


    if (pool_num + 9 < sp->pool_limit){
        auto rate4 = rates.find(pool_num + 8);
        auto amount9 = (double)quants_forecast3 * (double)rate4 -> sell_rate / (double)sp -> quants_precision;
        forecast4 = asset(amount9, root_symbol);
        quants_forecast4 = quants_forecast3 * rate4 -> sell_rate / rate4 -> buy_rate;

        forcasts.push_back(forecast4);
    }
    

    if (calculate_first == false){
        if (pool_num + 10 < sp->pool_limit){
            auto rate45 = rates.find(pool_num + 9);
            // if (acc->sale_is_enabled == true) {
                
            //     double convert_amount = (double)quants_forecast3 * (double)rate45->convert_rate / (double)sp->quants_precision;


            //     if (acc -> sale_mode == "direct"_n){
            //         forecast45 = asset(available_amount.amount, _POWER);
            //     }

                
            
            // } else {
            
                forecast45 = asset(uint64_t((double)forecast4.amount * (double)(HUNDR_PERCENT - sp -> loss_percent) / (double)HUNDR_PERCENT), root_symbol) ;
            
            // }

            forcasts.push_back(forecast45);
        }


        if (pool_num + 11 < sp->pool_limit){
            auto rate4 = rates.find(pool_num + 10);
            auto amount11 = (double)quants_forecast4 * (double)rate4 -> sell_rate / (double)sp -> quants_precision;
            eosio::asset forecast4 = asset(amount11, root_symbol);
            forcasts.push_back(forecast4);
        }
    }


    return forcasts;
};



[[eosio::action]] void unicore::withdrdacinc(eosio::name username, eosio::name host){
    require_auth(username);
    dacs_index dacs(_me, host.value);

    auto dac = dacs.find(username.value);

    eosio::check(dac != dacs.end(), "DAC is not found");
    
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);

    auto root_symbol = acc->get_root_symbol();
    eosio::asset to_pay = dac -> income;

    if (to_pay.amount > 0) {

        uint64_t vesting_seconds = unicore::getcondition(host, "teamvestsecs");
        
        if (vesting_seconds > 0) {

            make_vesting_action(username, host, acc->root_token_contract, to_pay, vesting_seconds, "teamwithdraw"_n);

        } else {

            action(
                permission_level{ _me, "active"_n },
                acc->root_token_contract, "transfer"_n,
                std::make_tuple( _me, username, to_pay, std::string("DAC income")) 
            ).send();

        }

        dacs.modify(dac, username, [&](auto &d){
            d.income = asset(0, root_symbol);
            d.income_in_segments = 0;
            d.withdrawed += to_pay;
            d.last_pay_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
        });    
    };
    

}




[[eosio::action]] void unicore::adddac(eosio::name username, eosio::name host, uint64_t weight, eosio::name limit_type, eosio::asset income_limit, std::string title, std::string descriptor) {
    
    account_index accounts(_me, _me.value);
    dacs_index dacs(_me, host.value);

    eosio::check( is_account( username ), "user account does not exist");

    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "Host is not found");
    
    require_auth(acc -> architect);
    
    auto root_symbol = acc->get_root_symbol();
    
    auto dac = dacs.find(username.value);
   

    if (dac == dacs.end()){
        dacs.emplace(acc -> architect, [&](auto &d){
            d.dac = username;
            d.weight = weight;
            d.income = asset(0, root_symbol);
            // d.income_limit = asset(0, root_symbol);
            d.withdrawed = asset(0, root_symbol);
            d.income_in_segments = 0;
            d.role = title;
            d.description = descriptor;
            d.limit_type = limit_type;
            d.income_limit = income_limit;
            d.last_pay_at = eosio::time_point_sec(0);
            d.created_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
        });

        accounts.modify(acc, acc -> architect, [&](auto &h){
            h.total_dacs_weight += weight;
        });       

    } else {
        int64_t new_weight = weight - dac->weight;

        dacs.modify(dac, acc -> architect, [&](auto &d){
            d.weight += new_weight;
            d.role = title;
        });

        accounts.modify(acc, acc -> architect, [&](auto &h){
            h.total_dacs_weight += new_weight;
        });        

    }

};

[[eosio::action]] void unicore::rmdac(eosio::name username, eosio::name host) {
    require_auth(host);
    account_index accounts(_me, _me.value);
    dacs_index dacs(_me, host.value);

    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "Host is not found");
    auto dac = dacs.find(username.value);
    
    eosio::check(dac != dacs.end(), "DAC is not found");

    accounts.modify(acc, host, [&](auto &h){
        h.total_dacs_weight -= dac->weight;
    });
    
    if (dac->income.amount > 0)
        action(
            permission_level{ _me, "active"_n },
            acc->root_token_contract, "transfer"_n,
            std::make_tuple( _me, username, dac->income, std::string("DAC income before remove")) 
        ).send();

    dacs.erase(dac);
};




/**
 * @brief      Публичный метод возврата баланса протоколу. 
 * Вывод средств возможен только для полностью обновленных (актуальных) балансов. Производит обмен Юнитов на управляющий токен и выплачивает на аккаунт пользователя. 
 * 
 * Производит расчет реферальных вознаграждений, генерируемых выплатой, и отправляет их всем партнерам согласно установленной формы.
 * 
 * Производит финансовое распределение между управляющими компаниями и целевым фондом сообщества. 
 * 
 * Каждый последующий пул, который участник проходит в качестве победителя, генеририрует бизнес-доход, который расчитывается исходя из того, что в текущий момент, средств всех проигравших полностью достаточно наа выплаты всем победителям с остатком. Этот остаток, в прапорции Юнитов пользователя и общего количества Юнитов в раунде, позволяет расчитать моментальную выплату, которая может быть изъята из системы при сохранении абсолютного балланса. 
 * 
 * Изымаемая сумма из общего котла управляющих токенов, разделяется на три потока, определяемые двумя параметрами: 
 * - Процент выплат на рефералов. Устанавливается в диапазоне от 0 до 100. Отсекает собой ровно ту часть пирога, которая уходит на выплаты по всем уровням реферальной структуры согласно ее формы. 
 * - Процент выплат на корпоративных управляющих. Устанавливается в диапазоне от 0 до 100. 
 * - Остаток от остатка распределяется в фонд целей сообщества.
 * 
 * Таким образом, коэффициенты позволяют распределять единый системный фонд по окружности с тремя секторами, где каждый сектор есть фонд со своим назначением.  

 * Например, если общий доход от движения баланса пользователя по спирали составляет 100 USD, а коэфициенты распределения Рефералов и Корпоративных Управляющих равняются соответственно по 0.5 (50%), то все рефералы получат по 33$, все управляющие получат по 33$, и еще 33$ попадет в фонд целей сообщества. 1$ останется в качестве комиссии округления на делении у протокола. 
 * 
 * 
 * @param[in]  op    The operation
 */

[[eosio::action]] void unicore::withdrawsold(eosio::name username, eosio::name host, uint64_t balance_id){
    //TODO Контроль выплат всем
    //TODO рефакторинг

    //Если системный доход не используется - УДАЛИТЬ

    // require_auth(username);
    eosio::check(has_auth(username) || has_auth(_self), "missing required authority");
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "host is not found");
    // eosio::name main_host = acc->get_ahost();
    
    auto root_symbol = acc->get_root_symbol();
    balance_index balance(_me, host.value);
    
    auto bal = balance.find(balance_id);
    eosio::check(bal != balance.end(), "Balance is not found");
    auto main_host = bal -> get_ahost();
    

    unicore::refresh_state(host);
    
    eosio::check(bal -> status != "process"_n, "Balance is not on a sale");
    eosio::check(bal -> solded_for.amount > 0, "Balance not have solded amount");

    action(
        permission_level{ _me, "active"_n },
        acc->root_token_contract, "transfer"_n,
        std::make_tuple( _me, username, bal -> solded_for, std::string("User Solded Withdraw")) 
    ).send();

    if (bal -> compensator_amount.amount > 0) {

        balance.modify(bal, _me, [&](auto &b){
            b.solded_for = asset(0, root_symbol);
        });

    } else {
        balance.erase(bal);
    }
}


/**
 * @brief      Публичный метод возврата баланса протоколу. 
 * Вывод средств возможен только для полностью обновленных (актуальных) балансов. Производит обмен Юнитов на управляющий токен и выплачивает на аккаунт пользователя. 
 * 
 * Производит расчет реферальных вознаграждений, генерируемых выплатой, и отправляет их всем партнерам согласно установленной формы.
 * 
 * Производит финансовое распределение между управляющими компаниями и целевым фондом сообщества. 
 * 
 * Каждый последующий пул, который участник проходит в качестве победителя, генеририрует бизнес-доход, который расчитывается исходя из того, что в текущий момент, средств всех проигравших полностью достаточно наа выплаты всем победителям с остатком. Этот остаток, в прапорции Юнитов пользователя и общего количества Юнитов в раунде, позволяет расчитать моментальную выплату, которая может быть изъята из системы при сохранении абсолютного балланса. 
 * 
 * Изымаемая сумма из общего котла управляющих токенов, разделяется на три потока, определяемые двумя параметрами: 
 * - Процент выплат на рефералов. Устанавливается в диапазоне от 0 до 100. Отсекает собой ровно ту часть пирога, которая уходит на выплаты по всем уровням реферальной структуры согласно ее формы. 
 * - Процент выплат на корпоративных управляющих. Устанавливается в диапазоне от 0 до 100. 
 * - Остаток от остатка распределяется в фонд целей сообщества.
 * 
 * Таким образом, коэффициенты позволяют распределять единый системный фонд по окружности с тремя секторами, где каждый сектор есть фонд со своим назначением.  

 * Например, если общий доход от движения баланса пользователя по спирали составляет 100 USD, а коэфициенты распределения Рефералов и Корпоративных Управляющих равняются соответственно по 0.5 (50%), то все рефералы получат по 33$, все управляющие получат по 33$, и еще 33$ попадет в фонд целей сообщества. 1$ останется в качестве комиссии округления на делении у протокола. 
 * 
 * 
 * @param[in]  op    The operation
 */

[[eosio::action]] void unicore::withdraw(eosio::name username, eosio::name host, uint64_t balance_id){
    //TODO Контроль выплат всем
    //TODO рефакторинг

    //Если системный доход не используется - УДАЛИТЬ

    // require_auth(username);
    eosio::check(has_auth(username) || has_auth(_self), "missing required authority");

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "host is not found");
    // eosio::name main_host = acc->get_ahost();
    
    auto root_symbol = acc->get_root_symbol();
    balance_index balance(_me, host.value);
    
    auto bal = balance.find(balance_id);
    eosio::check(bal != balance.end(), "Balance is not found");
    auto main_host = bal -> get_ahost();
    

    unicore::refresh_state(host);
    
    pool_index pools(_me, host.value);
    rate_index rates(_me, main_host.value);
    cycle_index cycles(_me, host.value);

    spiral_index spiral(_me, main_host.value);
    sincome_index sincome(_me, host.value);
    std::vector<eosio::asset> forecasts;
    
    auto sp = spiral.find(0);
    
    eosio::check(bal != balance.end(), "Balance is not found");
    
    auto pool = pools.find(bal->global_pool_id);
    
    auto cycle = cycles.find(pool -> cycle_num - 1);
    
    eosio::check(bal->last_recalculated_win_pool_id == cycle->finish_at_global_pool_id, "Cannot withdraw not refreshed balance. Refresh Balance first and try again.");

    auto next_cycle = cycles.find(pool -> cycle_num);
    auto has_new_cycle = false;
    if (next_cycle != cycles.end())
        has_new_cycle = true;

    auto last_pool = pools.find(cycle -> finish_at_global_pool_id );
    
    auto rate = rates.find(last_pool -> pool_num - 1 );
    auto next_rate = rates.find(last_pool -> pool_num );
    auto prev_rate = rates.find(last_pool -> pool_num - 2);
    
    eosio::check(last_pool -> remain_quants <= pool->total_quants, "Prevented withdraw. Only BP can restore this balance");
    
    eosio::check(bal -> status == "process"_n, "This balance stay on sale and can be withdrawed by this method. Use withdrawsold method");
    // eosio::check(bal -> available.amount > 0, "Cannot withdraw a zero balance. Please, write to help-center for resolve it");

    eosio::check(bal -> withdrawed == false, "Balance is already withdrawed");

    uint64_t pools_in_cycle = cycle -> finish_at_global_pool_id - cycle -> start_at_global_pool_id + 1;
    /**
     * Номинал
     * Номинал выплачивается если выполняется одно из условий: 
     *  - баланс куплен в одном из первых двух пулов, а текущий пул не выше второго;
     *  - баланс куплен в текущем пуле;
     *  - баланс куплен в последнем из возможных пулов;

     * 
     */
    // eosio::check(bal->available.amount > 0, "Cannot withdraw zero balance. Please, write to help-center for resolve it");
    
    if (((acc -> current_pool_num == pool -> pool_num ) && (acc -> current_cycle_num == pool -> cycle_num)) || \
        ((pool -> pool_num < 3) && (pools_in_cycle < 3)) || (has_new_cycle && (pool->pool_num == last_pool -> pool_num)))

    {
        action(
            permission_level{ _me, "active"_n },
            acc->root_token_contract, "transfer"_n,
            std::make_tuple( _me, username, bal -> purchase_amount, std::string("User Withdraw")) 
        ).send();

        action(
            permission_level{ _me, "active"_n },
            _me, "emitpower"_n,
            std::make_tuple( host , username, -bal->if_convert.amount, false) 
        ).send();

        if (pool -> pool_num < 3){

            pools.modify(pool, _me, [&](auto &p){
                p.remain_quants = std::min(pool-> remain_quants + bal -> quants_for_sale, pool->total_quants);
                
                uint64_t floated_remain_quants = p.remain_quants / sp -> quants_precision * sp -> quants_precision;

                p.filled = asset(( pool->total_quants - floated_remain_quants) / sp -> quants_precision * pool->quant_cost.amount, root_symbol);
                p.filled_percent = (pool->total_quants - p.remain_quants)  * HUNDR_PERCENT / pool->total_quants;
                p.remain = p.pool_cost - p.filled;


                auto current_rate = rates.find(bal -> pool_num - 1 );
                auto current_next_rate = rates.find(bal -> pool_num);
            
                unicore::change_bw_trade_graph(host, pool->id, pool->cycle_num, pool->pool_num, current_rate->buy_rate, current_next_rate->buy_rate, pool->total_quants, p.remain_quants, pool -> color);  
                
            }); 
            unicore::add_coredhistory(host, username, pool -> id, bal->purchase_amount, "nominal", "");
        } else {

            pools.modify(last_pool, _me, [&](auto &p){
                p.remain_quants = std::min(last_pool-> remain_quants + bal -> quants_for_sale, last_pool->total_quants);
                
                uint64_t floated_remain_quants = p.remain_quants / sp -> quants_precision * sp -> quants_precision;

                p.filled = asset(( last_pool->total_quants - floated_remain_quants) / sp -> quants_precision * last_pool->quant_cost.amount, root_symbol);
                
                p.filled_percent = (last_pool->total_quants - p.remain_quants) * HUNDR_PERCENT / last_pool->total_quants ;
                p.remain = p.pool_cost - p.filled;
        
                unicore::change_bw_trade_graph(host, last_pool->id, last_pool->cycle_num, last_pool->pool_num, rate->buy_rate, next_rate->buy_rate, last_pool->total_quants, p.remain_quants, last_pool -> color);  
            
            });
            unicore::add_coredhistory(host, username, last_pool -> id, bal->purchase_amount, "nominal", "");
        }
        
        unicore::add_user_stat("withdraw"_n, username, acc->root_token_contract, bal->purchase_amount, bal->available);
        unicore::add_host_stat("withdraw"_n, username, host, bal -> purchase_amount);
        unicore::add_host_stat2("nominal"_n, username, host, bal -> purchase_amount);
        unicore::add_core_stat("nominal"_n, host, bal -> purchase_amount);

        balance.erase(bal);
        
    } else  { 
    /**
     * Выигрыш или проигрыш.
     * Расчет производится на основе сравнения текущего цвета пула с цветом пула входа. При совпадении цветов - баланс выиграл. При несовпадении - проиграл. 
     * 
     */
        auto amount = bal -> available;
        
        if (amount.amount > 0)
            action(
                permission_level{ _me, "active"_n },
                acc->root_token_contract, "transfer"_n,
                std::make_tuple( _me, username, amount, std::string("User Withdraw")) 
            ).send();
    
        uint64_t quants_from_reserved;
        
        // action(
        //     permission_level{ _me, "active"_n },
        //     _me, "emitpower"_n,
        //     std::make_tuple( host , username, -bal->if_convert.amount, false) 
        // ).send();
        
        if (bal -> win == true) {

            /**
             * Выигрыш.
             * Инициирует распределение реферальных вознаграждений и выплаты во все фонды.
             */
            print(" bal -> ref_amount: ", bal->ref_amount);

            auto converted_quants = bal->quants_for_sale * rate -> sell_rate / rate -> buy_rate;
            
            auto sinc = sincome.find(acc->current_pool_id);
            
            //TODO may be corrent max with total prev element
            //(if payed before, max should decrease)
    
            market_index market(_me, host.value);
            auto itr = market.find(0);
            auto liquid_power = acc->total_shares - itr->base.balance.amount;

            /**
             * Все кванты победителей должны быть возвращены в пулы. 
             */
            
            uint64_t remain_quants = std::min(last_pool -> remain_quants + converted_quants, last_pool->total_quants);
            pools.modify(last_pool, _me, [&](auto &p){
                p.total_win_withdraw = last_pool -> total_win_withdraw + amount;
                p.remain_quants = remain_quants;

                p.filled = asset( (last_pool->total_quants / sp -> quants_precision - p.remain_quants / sp -> quants_precision) * last_pool->quant_cost.amount, root_symbol);
                p.filled_percent = (last_pool->total_quants - p.remain_quants) * HUNDR_PERCENT / last_pool->total_quants ;
                p.remain = p.pool_cost - p.filled;   
            });

            unicore::change_bw_trade_graph(host, last_pool->id, last_pool->cycle_num, last_pool->pool_num, rate->buy_rate, next_rate->buy_rate, last_pool->total_quants, remain_quants, last_pool -> color);
            unicore::add_coredhistory(host, username, last_pool -> id, amount, "w-withdraw", "");
            
            unicore::add_user_stat("withdraw"_n, username, acc->root_token_contract, bal->purchase_amount, bal->available);
            unicore::add_host_stat("withdraw"_n, username, host, bal->purchase_amount);
            
            

        } else {
            eosio::check(false, "balance cannot be withdrawed directly");
            pools.modify(last_pool, _me, [&](auto &p){
                 p.total_loss_withdraw = last_pool -> total_loss_withdraw + amount;               
            });
            
            
            // unicore::add_coredhistory(host, username, last_pool -> id, bal->if_convert, "convert", "");    
            
            unicore::add_user_stat("withdraw"_n, username, acc->root_token_contract, bal->purchase_amount, bal->available);
            unicore::add_host_stat("withdraw"_n, username, host, bal->purchase_amount);
        
            
            //JUST WITHDRAW LOSE AMOUNT
            unicore::add_coredhistory(host, username, last_pool -> id, amount, "l-withdraw", "");  

        }
        
        balance.erase(bal);
        // check_good_status(host, username, bal -> purchase_amount - bal -> available);

        
    
    }
};


    
    void unicore::spread_to_funds(eosio::name code, eosio::name host, eosio::asset total, std::string comment) {
        gpercents_index gpercents(_me, _me.value);
        auto syspercent = gpercents.find("system"_n.value);
        eosio::check(syspercent != gpercents.end(), "Contract is not active");

        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        auto main_host = acc->get_ahost();
        auto root_symbol = acc->get_root_symbol();
        
        eosio::check(root_symbol == total.symbol, "Wrong symbol for distribution");
        eosio::check(code == acc->root_token_contract, "Wrong root token for this host");
    
        eosio::asset total_dac_asset;
        eosio::asset total_cfund_asset;
        eosio::asset total_hfund_asset;
        eosio::asset total_sys_asset;
        

        spiral_index spiral(_me, host.value);
        auto sp = spiral.find(0);

        rate_index rates(_me, host.value);
        auto start_rate = rates.find(acc -> current_pool_num - 1);
        auto middle_rate = rates.find(acc -> current_pool_num);
        auto finish_rate = rates.find(acc -> current_pool_num + 1);

        // uint64_t total = uint64_t((double)quants * (double)start_rate -> buy_rate) / sp -> quants_precision;
        // print("buy_rate", start_rate -> buy_rate);


        double total_ref = ((double)total.amount * (double)(acc->referral_percent)) / ((double)HUNDR_PERCENT);
        double total_ref_min_sys = total_ref * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
        double total_ref_sys = total_ref * syspercent -> value / HUNDR_PERCENT;


        double total_dac = (double)total.amount * (double)(acc->dacs_percent) / (double)HUNDR_PERCENT;
        double total_dac_min_sys = total_dac * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
        double total_dac_sys = total_dac * syspercent -> value/ HUNDR_PERCENT;
    

        double total_cfund = (double)total.amount * (double)(acc->cfund_percent) / ((double)HUNDR_PERCENT);
        double total_cfund_min_sys = total_cfund * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
        double total_cfund_sys = total_cfund * syspercent -> value/ HUNDR_PERCENT;


        double total_hfund = (double)total.amount * (double)(acc->hfund_percent) / (double)HUNDR_PERCENT;  
        double total_hfund_min_sys = total_hfund * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
        double total_hfund_sys = total_hfund * syspercent -> value/ HUNDR_PERCENT;
    

        eosio::asset asset_ref_amount = asset((uint64_t)total_ref_min_sys, root_symbol);

        
        total_dac_asset = asset((uint64_t)total_dac_min_sys, root_symbol);
        total_cfund_asset = asset((uint64_t)total_cfund_min_sys, root_symbol);
        total_hfund_asset = asset((uint64_t)total_hfund_min_sys, root_symbol);
        
        
        double total_sys = total_dac_sys + total_cfund_sys + total_hfund_sys + total_ref_sys;
        total_sys_asset = asset((uint64_t)total_sys, root_symbol);


        spreads_index spreads(_me, host.value);
        
        spreads.emplace(_me, [&](auto &s){
            s.id = spreads.available_primary_key();
            s.timestamp = eosio::current_time_point();
            s.comment = comment;
            s.total = total;
            s.ref_amount = asset_ref_amount;
            s.dacs_amount = total_dac_asset;
            s.hfund_amount = total_hfund_asset;
            s.cfund_amount = total_cfund_asset;
            s.sys_amount = total_sys_asset;
        });        

        //REFS 
        
        if (total_dac_asset.amount > 0) {
            unicore::spread_to_dacs(host, total_dac_asset, code);
        }

        if (total_hfund_asset.amount > 0) {
            powerstat_index powerstats(_me, host.value);
            auto first_window = powerstats.find(0);

            if (first_window != powerstats.end()) {

                uint64_t now_secs = eosio::current_time_point().sec_since_epoch();
                uint64_t first_window_start_secs = first_window -> window_open_at.sec_since_epoch();
                uint64_t cycle = (now_secs - first_window_start_secs) / _WINDOW_SECS;

                auto current_window = powerstats.find(cycle);
                
                if (current_window != powerstats.end()) {
                    powerstats.modify(current_window, _me, [&](auto &ps) {
                        ps.total_available += total_hfund_asset;
                        ps.total_remain += total_hfund_asset;
                        ps.total_partners_available += asset_ref_amount;
                    });
               
                  
                } else {
                  //emplace window if something happen with auto-refresh state

                  powerstats.emplace(_me, [&](auto &ps) {
                      ps.id = cycle;
                      ps.window_open_at = eosio::time_point_sec(first_window_start_secs + _WINDOW_SECS * cycle);
                      ps.window_closed_at = eosio::time_point_sec(first_window_start_secs + _WINDOW_SECS * (cycle + 1));
                      ps.liquid_power = acc -> total_shares;
                      ps.total_available = total_hfund_asset;
                      ps.total_remain = total_hfund_asset;
                      ps.total_distributed = asset(0, root_symbol);
                      ps.total_partners_available = asset_ref_amount;
                      ps.total_partners_distributed = asset(0, root_symbol);
                  });

                }
            } else {

                powerstats.emplace(_me, [&](auto &ps){
                    ps.id = 0;
                    ps.window_open_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
                    ps.window_closed_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + _WINDOW_SECS);
                    ps.liquid_power = acc -> total_shares;
                    ps.total_available = total_hfund_asset;
                    ps.total_remain = total_hfund_asset;
                    ps.total_distributed = asset(0, root_symbol);
                    ps.total_partners_available = asset_ref_amount;
                    ps.total_partners_distributed = asset(0, root_symbol);
                }); 
                
            };
          };
        
        if (total_cfund_asset.amount > 0) {
            emission_index emis(_me, host.value);
            auto emi = emis.find(host.value);
            eosio::check(emi != emis.end(), "Emission pool for current host is not found");
            
            emis.modify(emi, _me, [&](auto &e) {
                e.fund += total_cfund_asset;
            });
        }
        
        if (total_sys_asset.amount > 0) {
            // Выплаты на системный аккаунт сообщества. 
            action(
                permission_level{ _me, "active"_n },
                "eosio"_n, "inprodincome"_n,
                std::make_tuple( acc -> root_token_contract, total_sys_asset) 
            ).send();
        };


    }

    void unicore::spread_to_refs(eosio::name host, eosio::name username, eosio::asset spread_amount, eosio::asset from_amount, eosio::name token_contract){
        partners2_index refs(_partners, _partners.value);
        auto ref = refs.find(username.value);

        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        
        eosio::check(acc -> root_token_contract == token_contract, "Wrong token contract");

        uint64_t usdtwithdraw = unicore::getcondition(host, "usdtwithdraw");

        bool usdt_withdraw = usdtwithdraw == 0 ? false : true;
        //Для выплат в USDT получаем курс валюты ядра к USD
        double sys_rate = 0; //system USDT rate

        if (usdt_withdraw) {
            usdrates_index usd_rates(_p2p, _p2p.value);
            auto rates_by_contract_and_symbol = usd_rates.template get_index<"byconsym"_n>();
            auto contract_and_symbol_index = combine_ids(acc->root_token_contract.value, spread_amount.symbol.code().raw());
            auto usd_rate = rates_by_contract_and_symbol.find(contract_and_symbol_index);
            sys_rate = usd_rate -> rate;
        };
        //
        
        eosio::name referer;
        uint8_t count = 1;
            
        if ((ref != refs.end()) && ((ref->referer).value != 0)) {
            referer = ref->referer;
            eosio::name prev_ref = ""_n;
            eosio::asset paid = asset(0, spread_amount.symbol);
        
            /**
             * Вычисляем размер выплаты для каждого уровня партнеров и производим выплаты.
             */

            for (auto level : acc->levels) {
                
                if ((ref != refs.end()) && ((ref->referer).value != 0)) { 
                    uint64_t to_ref_segments = spread_amount.amount * level / 100 / ONE_PERCENT;
                    eosio::asset to_ref_amount = asset(to_ref_segments, spread_amount.symbol);
                    
                    eosio::asset to_ref_usdt_amount;

                    if (usdt_withdraw) {
                        to_ref_usdt_amount = asset(to_ref_amount.amount * sys_rate, _USDT);
                    };

                    refbalances_index refbalances(_me, referer.value);
                    
                    refbalances.emplace(_me, [&](auto &rb){
                        rb.id = refbalances.available_primary_key();
                        rb.timepoint_sec = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
                        rb.host = host;
                        rb.refs_amount = spread_amount;
                        rb.win_amount = from_amount;
                        rb.amount = usdt_withdraw ? to_ref_usdt_amount: to_ref_amount;
                        rb.from = username;
                        rb.level = count;
                        rb.percent = level;
                        rb.cashback = ref->cashback;
                        rb.segments = (double)to_ref_segments * TOTAL_SEGMENTS;
                    });
                    paid += to_ref_amount;
                    
                    prev_ref = referer;
                    ref = refs.find(referer.value);
                    
                    referer = ref->referer;
                    
                    count++;

                } else {

                    break;

                }
            };

            /**
             * Все неиспользуемые вознаграждения с вышестояющих уровней отправляются на пользователя 
             * Все неиспользуемые вознаграждения с вышестояющих уровней отправляются на компании
             */
            eosio::asset back_to_host = spread_amount - paid;
            
            if (back_to_host.amount > 0) {
                // if (prev_ref != ""_n) {
                    
                //     refbalances_index refbalances2(_me, prev_ref.value);
                    
                //     uint128_t to_ref_segments = back_to_host.amount * TOTAL_SEGMENTS;

                //     refbalances2.emplace(_me, [&](auto &rb){
                //         rb.id = refbalances2.available_primary_key();
                //         rb.timepoint_sec = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
                //         rb.host = host;
                //         rb.refs_amount = back_to_host;
                //         rb.win_amount = back_to_host;
                //         rb.amount = back_to_host;
                //         rb.from = username;
                //         rb.level = count - 1;
                //         rb.percent = 0;
                //         rb.cashback = 0;
                //         rb.segments = (double)to_ref_segments;
                //     }); 

                // } else {

                    unicore::spread_to_dacs(host, back_to_host, acc -> root_token_contract);    

                // };
                

                
            }


            
        } else {
            /**
             * Если рефералов у пользователя нет, то переводим все реферальные средства пользователю.
             * * Если рефералов у пользователя нет, то переводим все реферальные средства компании.
             */
            if (spread_amount.amount > 0){
                unicore::spread_to_dacs(host, spread_amount, acc -> root_token_contract);
                // refbalances_index refbalances(_me, username.value);
                
                // uint128_t to_ref_segments = spread_amount.amount * TOTAL_SEGMENTS;

                // refbalances.emplace(_me, [&](auto &rb){
                //     rb.id = refbalances.available_primary_key();
                //     rb.timepoint_sec = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
                //     rb.host = host;
                //     rb.refs_amount = spread_amount;
                //     rb.win_amount = spread_amount;
                //     rb.amount = spread_amount;
                //     rb.from = username;
                //     rb.level = 0;
                //     rb.percent = 0;
                //     rb.cashback = 0;
                //     rb.segments = (double)to_ref_segments;
                // });
            }
       
        }

    };


    [[eosio::action]] void unicore::refrollback(eosio::name host, eosio::name username, uint64_t balance_id){
        require_auth(host);
        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        auto root_symbol = acc->get_root_symbol();
        
        eosio::check(acc -> username == host, "Only host can delete ref balance");

        refbalances_index refbalances(_me, username.value);
        auto rb = refbalances.find(balance_id);

        eosio::check(refbalances.end() != rb, "Balance is not found");

        refbalances.erase(rb);
    }


    void unicore::spread_to_dacs(eosio::name host, eosio::asset amount, eosio::name contract) {

        dacs_index dacs(_me, host.value);
        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        auto root_symbol = acc->get_root_symbol();
        eosio::check(contract == acc -> root_token_contract, "Wrong token contract for spread");
        eosio::check(amount.symbol == root_symbol, "System error on spead to dacs");

        auto dac = dacs.begin();
        
        if (dac != dacs.end()){
            while(dac != dacs.end()) {
        
                eosio::asset amount_for_dac = asset(amount.amount * dac -> weight / acc-> total_dacs_weight, root_symbol);
                
                dacs.modify(dac, _me, [&](auto &d){

                    uint128_t dac_income_in_segments = amount_for_dac.amount * TOTAL_SEGMENTS;

                    double income = double(dac ->income_in_segments + dac_income_in_segments) / (double)TOTAL_SEGMENTS;

                    d.income = asset(uint64_t(income), root_symbol);
                    d.income_in_segments += amount_for_dac.amount * TOTAL_SEGMENTS;
                
                });
                
                dac++;
                
            }
        } else {
            //transfer to host
            action(
                permission_level{ _me, "active"_n },
                acc->root_token_contract, "transfer"_n,
                std::make_tuple( _me, host, amount, std::string("host income")) 
            ).send();
        }
    }

   
     void unicore::add_ref_stat(eosio::name username, eosio::name contract, eosio::asset withdrawed){
        refstat_index refstats(_me, withdrawed.symbol.code().raw());

        auto stat_index = refstats.template get_index<"byconuser"_n>();

        auto stat_ids = combine_ids(contract.value, username.value);
        auto user_stat = stat_index.find(stat_ids);

        if (user_stat == stat_index.end()){
            //emplace
            refstats.emplace(_me, [&](auto &u){
                u.id = refstats.available_primary_key();
                u.username = username;
                u.contract = contract;
                u.symbol = withdrawed.symbol.code().to_string();
                u.precision = withdrawed.symbol.precision();
                u.withdrawed = withdrawed;
            });

        } else {
            
            stat_index.modify(user_stat, _me, [&](auto &u) {
                u.withdrawed += withdrawed;                
            });
        }


    }

     void unicore::add_core_stat(eosio::name type, eosio::name host, eosio::asset amount){
        corestat_index corestat(_me, _me.value);

        auto stat = corestat.find(host.value);

        if (stat == corestat.end()){
            if (type == "deposit"_n) {
                corestat.emplace(_me, [&](auto &u){
                    u.hostname = host;
                    u.volume = amount;
                });
            };
        } else {
            
            corestat.modify(stat, _me, [&](auto &u) {
                if (type == "nominal"_n) {
                    u.volume =  -amount;
                };
            });
        }
    }


     void unicore::add_host_stat(eosio::name type, eosio::name username, eosio::name host, eosio::asset amount){
        hoststat_index hoststat(_me, host.value);

        auto stat = hoststat.find(username.value);

        if (stat == hoststat.end()){
            //emplace
            hoststat.emplace(_me, [&](auto &u){
                u.username = username;
                u.blocked_now = amount;
            });

        } else {
            
            hoststat.modify(stat, _me, [&](auto &u) {
                if (type == "deposit"_n) {
                    u.blocked_now += amount;
                } else if (type == "withdraw"_n) {
                    u.blocked_now = stat -> blocked_now >= amount ? stat -> blocked_now - amount : asset(0, amount.symbol);
                };
                
            });
        }
    }



     void unicore::add_host_stat2(eosio::name type, eosio::name username, eosio::name host, eosio::asset amount){
        hoststat_index2 hoststat2(_me, host.value);

        auto stat = hoststat2.find(username.value);

        if (stat == hoststat2.end()){
            if (type == "deposit"_n) {
                hoststat2.emplace(_me, [&](auto &u){
                    u.username = username;
                    u.volume = amount;
                });
            };
        } else {
            hoststat2.modify(stat, _me, [&](auto &u) {
                if (type == "nominal"_n) {
                    u.volume =  - amount;
                };
            });
        }
    }



     void unicore::add_user_stat(eosio::name type, eosio::name username, eosio::name contract, eosio::asset nominal_amount, eosio::asset withdraw_amount){
        userstat_index userstat(_me, nominal_amount.symbol.code().raw());

        auto stat_index = userstat.template get_index<"byconuser"_n>();

        auto stat_ids = combine_ids(contract.value, username.value);
        auto user_stat = stat_index.find(stat_ids);

        if (user_stat == stat_index.end()){
            //emplace
            userstat.emplace(_me, [&](auto &u){
                u.id = userstat.available_primary_key();
                u.username = username;
                u.contract = contract;
                u.symbol = nominal_amount.symbol.code().to_string();
                u.precision = nominal_amount.symbol.precision();
                u.blocked_now = nominal_amount;
                u.total_nominal = nominal_amount;
                u.total_withdraw = asset(0, nominal_amount.symbol);
                
            });

        } else {
            
            stat_index.modify(user_stat, _me, [&](auto &u) {
                if (type == "deposit"_n){
                    u.blocked_now += nominal_amount;
                    u.total_nominal += nominal_amount;
                } else if (type == "withdraw"_n){
                    u.blocked_now -= nominal_amount;
                    u.total_withdraw += withdraw_amount;
                };
                
            });
        }


    }



/**
 * @brief Публичный метод включения сейла с хоста. Может быть использован только до вызова метода start при условии, что владелец контракта токена разрешил это. Активирует реализацию управляющего жетона из фонда владельца жетона в режиме финансового котла. 

 *
*/
[[eosio::action]] void unicore::changemode(eosio::name host, eosio::name mode) {
    require_auth(host);

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    
    eosio::check(acc != accounts.end(), "Host is not found");
    eosio::check(acc -> type == "tin"_n, "Cannot change mode after first change");

    accounts.modify(acc, host, [&](auto &h){
        h.type = mode;
    });
}




//PREPARE TO DELETE


/**
 * @brief Публичный метод перевода из глобального фонда

 *
*/
[[eosio::action]] void unicore::transfromgf(eosio::name to, eosio::name token_contract, eosio::asset quantity){
   
}

/**
 * @brief Публичный метод включения сейла с хоста. Может быть использован только до вызова метода start при условии, что владелец контракта токена разрешил это. Активирует реализацию управляющего жетона из фонда владельца жетона в режиме финансового котла. 

 *
*/
[[eosio::action]] void unicore::enablesale(eosio::name host, eosio::name token_contract, eosio::asset asset_on_sale, int64_t sale_shift, eosio::name sale_mode){
   
}


[[eosio::action]] void unicore::rmhosfrfund(uint64_t fund_id, eosio::name host){
    
}

/**
 * @brief Публичный метод подключения хоста к фонду
 *
*/
[[eosio::action]] void unicore::addhostofund(uint64_t fund_id, eosio::name host){
    
}


/**
 * @brief Публичный метод пополнения фонда
 *
*/
void unicore::add_asset_to_fund_action(eosio::name username, eosio::asset quantity, eosio::name code){
    
    require_auth(username);

}


 void unicore::add_sale_history(hosts acc, rate rate, spiral sp, eosio::asset amount){
    
    }

    void unicore::check_and_modify_sale_fund(eosio::asset amount, hosts acc){
       

    }


/**
 * @brief Публичный метод покупки по текущему курсу из числа квантов раунда.
 *
*/

eosio::asset unicore::buy_action(eosio::name username, eosio::name host, eosio::asset quantity, eosio::name code, bool modify_pool, bool transfer, bool spread_to_funds, eosio::asset summ = asset()){
    
    // account_index accounts(_me, _me.value);
    // auto acc = accounts.find(host.value);
    
    // eosio::check(acc != accounts.end(), "Host is not found");
    // eosio::check(acc -> sale_is_enabled == true, "Sale is disabled");
    
    // eosio::check(acc -> sale_mode == "counterpower"_n || acc->sale_mode == "counterunit"_n, "Impossible to buy");

    // eosio::name main_host = acc->get_ahost();
    // auto root_symbol = acc->get_root_symbol();

    // eosio::check(code == acc->root_token_contract, "Wrong root token for this host");
    // eosio::check(quantity.symbol == root_symbol, "Wrong root symbol for this host");

    // pool_index pools(_me, host.value);
    // rate_index rates(_me, host.value);

    // spiral_index spiral(_me, host.value);
    // auto sp = spiral.find(0);

    // // auto cycle = cycles.find(pool -> cycle_num - 1);
    // auto pool = pools.find(acc->current_pool_id);
    // auto rate = rates.find(acc->current_pool_num - 1);
    // auto next_rate = rates.find(acc->current_pool_num);

    // auto quant_cost = pool -> quant_cost;

    
    // eosio::check(quantity.amount >= quant_cost.amount, "Not enought quantity for buy 1 quant");
    // // eosio::check(quantity.amount % quant_cost.amount == 0, "You can purchase only whole quant");
    
    // uint64_t quants = quantity.amount * sp->quants_precision / rate->buy_rate / sp->quants_precision * sp->quants_precision;

    // //TODO make recursive and return summ of buys
    // // eosio::check(pool->remain_quants >= quants, "Not enought quants in target pool");
    // uint64_t quants_for_buy;

    // eosio::asset remain_asset = asset(0, root_symbol);

    // if (pool->remain_quants < quants) {

    //     quants_for_buy = pool->remain_quants;
    //     remain_asset = quantity - pool->remain;
        
    // } else {
        
    //     quants_for_buy = quants;
    
    // };

    // auto amount_in_quants = quants_for_buy  /  sp -> quants_precision * rate -> convert_rate;
    // eosio::asset quants_for_user_in_asset = asset(amount_in_quants, (acc->asset_on_sale).symbol);
    
    // if (modify_pool == true){

    //     pools.modify(pool, _me, [&](auto &p) {
    //         p.remain_quants = pool -> remain_quants - quants_for_buy;
    //         p.filled = asset(( pool -> total_quants - p.remain_quants) / sp -> quants_precision * pool->quant_cost.amount, root_symbol);
    //         p.filled_percent = (pool -> total_quants - p.remain_quants) * HUNDR_PERCENT / pool->total_quants;
    //         p.remain = p.pool_cost - p.filled;
    //         // p.creserved_quants += quants_for_buy; 
    //     });
    // }

    // unicore::add_sale_history(*acc, *rate, *sp, quants_for_user_in_asset);
    
    // if (transfer == true) {
    //     action(
    //         permission_level{ _me, "active"_n },
    //         acc->sale_token_contract, "transfer"_n,
    //         std::make_tuple( _me, username, quants_for_user_in_asset, std::string("Direct buy")) 
    //     ).send();
    // }

    // unicore::add_coredhistory(host, username, pool -> id, quants_for_user_in_asset, "burn", "");    
    // unicore::change_bw_trade_graph(host, pool -> id, pool -> cycle_num, pool -> pool_num, rate -> buy_rate, next_rate -> buy_rate, pool -> total_quants, pool -> remain_quants, pool -> color);
    
    // //spread amount calculated from quants for buy and part of system income:

    // unicore::refresh_state(host);
    
    // if (summ.amount > 0) {
    
    //     summ = asset(summ.amount + quants_for_user_in_asset.amount, (acc->asset_on_sale).symbol);
    
    // } else summ = quants_for_user_in_asset; 
    
    // if ( remain_asset.amount > 0 ) {

    //     buy_action(username, host, remain_asset, code, modify_pool, transfer, spread_to_funds, summ);  

    // }
    
    return summ;
}



/**
 * @brief Статичный метод создания фонда
 *
*/

void unicore::createfund(eosio::name token_contract, eosio::asset fund_asset, std::string descriptor){
    
}



/**
 * @brief Публичный метод покупки статуса участника для аккаунта (выкуп гостевого аккаунта у регистратора)
 *
*/

void unicore::buy_account(eosio::name username, eosio::name host, eosio::asset quantity, eosio::name code, eosio::name status){

    
}


/**
 * @brief Публичный метод сжигания квантов по текущему курсу из числа квантов раунда.
 *
*/

void unicore::burn_action(eosio::name username, eosio::name host, eosio::asset quantity, eosio::name code, eosio::name status){
    
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    
    eosio::check(acc != accounts.end(), "Host is not found");
    
    eosio::name main_host = acc->get_ahost();
    auto root_symbol = acc->get_root_symbol();

    eosio::check(code == acc->root_token_contract, "Wrong root token for this host");
    eosio::check(quantity.symbol == root_symbol, "Wrong root symbol for this host");

    pool_index pools(_me, host.value);
    rate_index rates(_me, host.value);

    spiral_index spiral(_me, host.value);
    auto sp = spiral.find(0);

    // auto cycle = cycles.find(pool -> cycle_num - 1);
    auto pool = pools.find(acc->current_pool_id);
    auto rate = rates.find(acc->current_pool_num - 1);
    auto next_rate = rates.find(acc->current_pool_num);

    auto quant_cost = pool -> quant_cost;

    // eosio::check(quantity.amount >= quant_cost.amount, "Not enought quantity for buy 1 quant");
    // eosio::check(quantity.amount % quant_cost.amount == 0, "You can purchase only whole quant");
    
    eosio::asset ref_amount = quantity * acc -> referral_percent / HUNDR_PERCENT;
    eosio::asset core_amount = quantity - ref_amount; 
    
    if (ref_amount.amount > 0) {
        unicore::spread_to_refs(host, username, ref_amount, quantity, acc -> root_token_contract);
    };

    uint128_t dquants = (uint128_t)core_amount.amount * (uint128_t)sp->quants_precision / (uint128_t)rate->buy_rate;
    uint64_t quants = dquants;
   

    //TODO make recursive and return summ of buys
    // eosio::check(pool->remain_quants >= quants, "Not enought quants in target pool");

    uint64_t quants_for_buy;

    eosio::asset remain_asset = asset(0, root_symbol);
    uint64_t remain_quants = 0;

    if (pool->remain_quants < quants) {

        quants_for_buy = pool->remain_quants;
        remain_asset = quantity - pool->remain;
        remain_quants = quants - pool->remain_quants;
        
    } else {
        
        quants_for_buy = quants;
    
    };

    
    pools.modify(pool, _me, [&](auto &p) {
        p.remain_quants = pool -> remain_quants - quants_for_buy;
        p.filled = asset(( pool -> total_quants - p.remain_quants) / sp -> quants_precision * pool->quant_cost.amount, root_symbol);
        p.filled_percent = (pool -> total_quants - p.remain_quants) * HUNDR_PERCENT / pool->total_quants;
        p.remain = p.pool_cost - p.filled;
        // p.creserved_quants += quants_for_buy; 
    });
    

    unicore::add_coredhistory(host, username, pool -> id, quantity, "burn", "");    
    unicore::change_bw_trade_graph(host, pool -> id, pool -> cycle_num, pool -> pool_num, rate -> buy_rate, next_rate -> buy_rate, pool -> total_quants, pool -> remain_quants, pool -> color);
    
    if (host == "core"_n)
        unicore::check_status(host, username, quantity, status);
    
    if (remain_asset.amount > 0) {
        unicore::spread_to_dacs(host, remain_asset, acc -> root_token_contract);
    }
    

    double power = (double)quantity.amount / (double)acc -> sale_shift * (double)sp->quants_precision;
    int64_t user_power = int64_t(power);

    action(
        permission_level{ _me, "active"_n },
        _me, "emitpower"_n,
        std::make_tuple( host , username, user_power, false) 
    ).send();

    unicore::refresh_state(host);
    
}


/**
 * @brief Публичный метод сжигания квантов по текущему курсу из числа квантов раунда.
 *
*/

void unicore::subscribe_action(eosio::name username, eosio::name host, eosio::asset quantity, eosio::name code, eosio::name status){
    
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    
    eosio::check(acc != accounts.end(), "Host is not found");
    
    eosio::name main_host = acc->get_ahost();
    auto root_symbol = acc->get_root_symbol();

    eosio::check(code == acc->root_token_contract, "Wrong root token for this host");
    eosio::check(quantity.symbol == root_symbol, "Wrong root symbol for this host");

    pool_index pools(_me, host.value);
    rate_index rates(_me, host.value);

    spiral_index spiral(_me, host.value);
    auto sp = spiral.find(0);

    // auto cycle = cycles.find(pool -> cycle_num - 1);
    auto pool = pools.find(acc->current_pool_id);
    auto rate = rates.find(acc->current_pool_num - 1);
    auto next_rate = rates.find(acc->current_pool_num);

    auto quant_cost = pool -> quant_cost;

    // eosio::check(quantity.amount >= quant_cost.amount, "Not enought quantity for buy 1 quant");
    // eosio::check(quantity.amount % quant_cost.amount == 0, "You can purchase only whole quant");
    
    eosio::asset ref_amount = quantity * acc -> referral_percent / HUNDR_PERCENT;
    eosio::asset core_amount = quantity - ref_amount; 
    
    if (ref_amount.amount > 0) {
        unicore::spread_to_refs(host, username, ref_amount, quantity, acc -> root_token_contract);
    };

    uint128_t dquants = (uint128_t)core_amount.amount * (uint128_t)sp->quants_precision / (uint128_t)rate->buy_rate;
    uint64_t quants = dquants;
   

    //TODO make recursive and return summ of buys
    // eosio::check(pool->remain_quants >= quants, "Not enought quants in target pool");

    uint64_t quants_for_buy;

    eosio::asset remain_asset = asset(0, root_symbol);
    uint64_t remain_quants = 0;

    if (pool->remain_quants < quants) {

        quants_for_buy = pool->remain_quants;
        remain_asset = quantity - pool->remain;
        remain_quants = quants - pool->remain_quants;
        
    } else {
        
        quants_for_buy = quants;
    
    };

    
    pools.modify(pool, _me, [&](auto &p) {
        p.remain_quants = pool -> remain_quants - quants_for_buy;
        p.filled = asset(( pool -> total_quants - p.remain_quants) / sp -> quants_precision * pool->quant_cost.amount, root_symbol);
        p.filled_percent = (pool -> total_quants - p.remain_quants) * HUNDR_PERCENT / pool->total_quants;
        p.remain = p.pool_cost - p.filled;
        // p.creserved_quants += quants_for_buy; 
    });
    

    unicore::add_coredhistory(host, username, pool -> id, quantity, "burn", "");    
    unicore::change_bw_trade_graph(host, pool -> id, pool -> cycle_num, pool -> pool_num, rate -> buy_rate, next_rate -> buy_rate, pool -> total_quants, pool -> remain_quants, pool -> color);
    
    if (host != "core"_n)
        unicore::check_subscribe(host, username, quantity, status);
    
    if (remain_asset.amount > 0) {
        unicore::spread_to_dacs(host, remain_asset, acc -> root_token_contract);
    }
    

    // double power = (double)quantity.amount / (double)acc -> sale_shift * (double)sp->quants_precision;
    // int64_t user_power = int64_t(power);

    // action(
    //     permission_level{ _me, "active"_n },
    //     _me, "emitpower"_n,
    //     std::make_tuple( host , username, user_power, false) 
    // ).send();

    unicore::refresh_state(host);
    
}


[[eosio::action]] void unicore::suggestrole(eosio::name username, std::string title, std::string descriptor) {
    

}



    eosio::asset unicore::calculate_asset_from_power(eosio::asset quantity, eosio::name host){
       
        return asset(0, _SYM);
    }




/**
 * @brief Публичный метод обмена баланса на жетон распределения по текущему курсу из числа квантов раунда.
 *
*/


[[eosio::action]] void unicore::convert(eosio::name username, eosio::name host, uint64_t balance_id){
    require_auth("eosio"_n);

    // account_index accounts(_me, _me.value);

    // pool_index pools(_me, host.value);
    // rate_index rates(_me, host.value);
    // spiral_index spiral(_me, host.value);

    // auto acc = accounts.find(host.value);
    // auto sp = spiral.find(0);

    // eosio::check(acc -> type == "fraction"_n, "Host mode should be fraction for convertation");
    
    // balance_index balances(_me, host.value);
    // auto bal = balances.find(balance_id);

    // eosio::check(bal != balances.end(), "balance is not found");

    // cycle_index cycles(_me, host.value);
    // auto cycle = cycles.find(bal-> cycle_num - 1);
    
    // eosio::check(bal->last_recalculated_win_pool_id == cycle->finish_at_global_pool_id, "Cannot convert not refreshed balance. Refresh Balance first and try again.");

    // eosio::check(bal -> win == 0, "Only lose balances can be converted to goal");
    // eosio::check(bal -> available < bal -> purchase_amount, "Only lose balances can be converted to goal");

    // eosio::asset target = bal->available * 150 / 100;


    // goals_index goals(_me, host.value);
    // uint64_t id = get_global_id("goals"_n);

    // goals.emplace("eosio"_n, [&](auto &g){
    //   g.id = id;
    //   g.parent_id = 0;
    //   g.creator = username;
    //   // g.benefactor = creator;
    //   g.status = "process"_n;
    //   g.created = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
    //   g.finish_at = eosio::time_point_sec(-1);
    //   g.host = host;
    //   g.title = "Целевой возврат средств с прибылью +50%";
    //   g.description = "";
    //   g.target = target;
    //   g.target1 = asset(0, target.symbol);
    //   g.target2 = asset(0, target.symbol);
    //   g.second_circuit_votes = 0;
    //   g.debt_amount = asset(0, target.symbol);
    //   g.withdrawed = asset(0, target.symbol);
    //   g.available = bal->available;
    //   g.meta = std::to_string(bal -> id);
    // });      

    // accounts.modify(acc, "eosio"_n, [&](auto &a) {
    //   a.total_goals = acc -> total_goals + 1;
    // });  

    // action(
    //     permission_level{ _me, "active"_n },
    //     _me, "setgoal"_n,
    //     std::make_tuple( bal -> owner, bal -> host, uint64_t(0), std::string("Целевой возврат"), std::string(""), target, std::string("")) 
    // ).send();

    // (eosio::name creator, eosio::name host, uint64_t parent_id, std::string title, std::string description, eosio::asset target, std::string meta){
    // 
    // auto rate = rates.find(acc->current_pool_num - 1);
    
    // eosio::check(bal->if_convert.symbol == _POWER, "Impossible convert this balance");

    // unicore::add_coredhistory(host, username, acc->current_pool_id, bal->if_convert, "convert", "");

    // accounts.modify(acc, _me, [&](auto &a){
    //     a.total_shares += bal -> if_convert.amount;
    // });

    // power3_index power(_me, host.value);
    // auto pexist = power.find(username.value);
    
    // if (pexist == power.end()) {

    //   power.emplace(_me, [&](auto &p) {
    //     p.username = username;
    //     p.power = bal->if_convert.amount;
    //     p.staked = bal->if_convert.amount;    
    //   });

        
    // } else {
    //     unicore::propagate_votes_changes(host, username, pexist->power, pexist->power + bal->if_convert.amount);
        
    //     power.modify(pexist, _me, [&](auto &p) {
    //         p.power += bal->if_convert.amount;
    //         p.staked += bal->if_convert.amount;
    //     });
    // };

    // balances.erase(bal);
 

}




/**
 * @brief Публичный метод распределения входящих токенов среди фондов хоста и партнёров реферала
 *
*/

void unicore::spread_action(eosio::name username, eosio::name host, eosio::asset quantity, eosio::name code){
    
}

  eosio::asset unicore::convert_to_power(eosio::asset quantity, eosio::name host){
       
        return asset(0, _SYM);
    }
