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


[[eosio::action]] void unicore::init(uint64_t system_income, uint64_t operator_income) {
    require_auth(_me);

    gpercents_index gpercents(_me, _me.value);

    eosio::check(system_income <= HUNDR_PERCENT, "system income should be less then 100%");
    eosio::check(operator_income <= HUNDR_PERCENT, "operator income should be less then 100%");

    auto gsys_income = gpercents.find("system"_n.value);

    if (gsys_income == gpercents.end()) {
        gpercents.emplace(_me, [&](auto &gp){
            gp.key = "system"_n;
            gp.value = system_income;
        });    
    } else {
        
        gpercents.modify(gsys_income, _me, [&](auto &gp){
            gp.value = system_income;
        });

    };

    auto op_income = gpercents.find("operator"_n.value);

    if (op_income == gpercents.end()){
        gpercents.emplace(_me, [&](auto &gp){
            gp.key = "operator"_n;
            gp.value = operator_income;
        });    
    } else {
        gpercents.modify(op_income, _me, [&](auto &gp){
            gp.value = operator_income;
        });

    };
    
    
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

eosio::asset unicore::emit(eosio::name host, eosio::asset host_income, eosio::asset max_income) {
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    auto root_symbol = acc->get_root_symbol();

    eosio::name ahost = acc->get_ahost();
        
    emission_index emis(_me, host.value);
    auto emi = emis.find(host.value);
    eosio::asset emi_fund = emi -> fund;

    auto main_host = acc->get_ahost();
    
    eosio::asset total_dac_asset = asset(0, host_income.symbol);
    eosio::asset total_cfund_asset = asset(0, host_income.symbol);
    eosio::asset total_hfund_asset = asset(0, host_income.symbol);
    eosio::asset total_sys_asset = asset(0, host_income.symbol);
    
    if (host_income.amount > 0) {
        gpercents_index gpercents(_me, _me.value);
        uint64_t syspercent = gpercents.find("system"_n.value) -> value;
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

        if (total_cfund_asset.amount > 0) {
            emis.modify(emi, _me, [&](auto &e) {
                emi_fund = emi_fund + total_cfund_asset;
                e.fund = emi_fund;
            });
        };


        if (total_dac_asset.amount > 0) {
            std::string memo = std::string("открыт новый раунд") + std::string(" в DAO ") + host.to_string(); 

            unicore::push_to_dacs_funds(host, total_dac_asset, acc -> root_token_contract, memo);
        };

        if (total_sys_asset.amount > 0) {
            // Выплаты на системный аккаунт сообщества. 

            uint64_t operator_percent = gpercents.find("operator"_n.value) -> value;
            eosio::asset operator_amount = total_sys_asset * operator_percent / HUNDR_PERCENT;

            eosio::asset witness_amount = total_sys_asset - operator_amount;
            
            if (witness_amount.amount > 0)
                action(
                    permission_level{ _me, "active"_n },
                    "eosio"_n, "inprodincome"_n,
                    std::make_tuple( acc -> root_token_contract, witness_amount) 
                ).send();


            if (operator_amount.amount > 0) {
                //TODO 50% to operator and 50% to him network
                //кто устанавливает процент? 
                //система

                eosio::asset ref_operator_amount = operator_amount / 2; 
                eosio::asset after_ref_operator_amount = operator_amount - ref_operator_amount;

                std::string memo = std::string("открыт новый раунд") + std::string(" в DAO ") + host.to_string() + std::string(" у оператора ") + acc -> hoperator.to_string(); 

                unicore::push_to_dacs_funds(host, after_ref_operator_amount, acc -> root_token_contract, memo);
                unicore::spread_to_refs(host, acc -> hoperator, after_ref_operator_amount, operator_amount, acc -> root_token_contract);                

                
            }
        };



        sincome_index sincome(_me, host.value);
        
        auto prev_sinc = sincome.find(acc -> current_pool_id);
        
        eosio::check(prev_sinc != sincome.end(), "system income object is not found");

        sincome.emplace(_me, [&](auto &s) {
            s.max = max_income;

            s.pool_id = acc->current_pool_id + 1;
            s.ahost = main_host;
            s.cycle_num = acc->current_cycle_num;
            s.pool_num = acc->current_pool_num + 1;
            s.total = prev_sinc -> total + total_sys_asset + total_dac_asset + total_cfund_asset + total_hfund_asset;
            s.paid_to_sys = prev_sinc -> paid_to_sys + total_sys_asset;
            s.paid_to_dacs = prev_sinc -> paid_to_dacs + total_dac_asset;
            s.paid_to_cfund = prev_sinc -> paid_to_cfund + total_cfund_asset;
            s.paid_to_hfund = prev_sinc -> paid_to_hfund + total_hfund_asset;
            s.paid_to_refs = prev_sinc -> paid_to_refs + asset(0, root_symbol);
        }); 

    } else {

        sincome_index sincome(_me, host.value);
        
        auto prev_sinc = sincome.find(acc -> current_pool_id);
        
        eosio::check(prev_sinc != sincome.end(), "system income object is not found");

        sincome.emplace(_me, [&](auto &s) {
            s.max = max_income;

            s.pool_id = acc->current_pool_id + 1;
            s.ahost = main_host;
            s.cycle_num = acc->current_cycle_num;
            s.pool_num = acc->current_pool_num + 1;
            
            s.total = prev_sinc -> total;
            
            s.paid_to_sys = prev_sinc -> paid_to_sys;
            s.paid_to_dacs = prev_sinc -> paid_to_dacs;
            s.paid_to_cfund = prev_sinc -> paid_to_cfund;
            s.paid_to_hfund = prev_sinc -> paid_to_hfund;
            s.paid_to_refs = prev_sinc -> paid_to_refs;
        }); 
 
    }

    //SPREAD TO GOALS        
    if (emi->percent > 0 && total_cfund_asset.amount > 0){
        goals_index goals(_me, host.value);
        eosio::asset em = unicore::adjust_goals_emission_pool(host, total_cfund_asset);  
        eosio::asset on_emission = emi_fund <= em ? emi_fund : em;
        eosio::asset by_votes = on_emission / 2;
        eosio::asset by_timestamp = on_emission - by_votes;
        eosio::asset fact_emitted = asset(0, on_emission.symbol);

        uint64_t limit = 30;

        if (by_votes.amount > 0) {
            
            std::vector<uint64_t> list_for_emit;
            uint64_t devider;

            auto idx_bv = goals.template get_index<"votes"_n>();
            
            auto i_bv = idx_bv.rbegin();

            uint64_t gcount = 0;
            uint64_t i = 0;
            
            while(i_bv != idx_bv.rend() && gcount != emi->gtop  && i <= limit) {
                if((i_bv -> status == "validated"_n) && (i_bv -> target.amount > 0)){
                    list_for_emit.push_back(i_bv->id);
                    gcount++;
                }
                i++;
                i_bv++;
            };        

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

        };


        if (by_timestamp.amount > 0) {
            std::vector<uint64_t> list_for_emit;
            uint64_t devider;
            auto index = goals.template get_index<"bystatus"_n>();
            
            auto goal = index.lower_bound("validated"_n.value);

            uint64_t gcount = 0;
            uint64_t i = 0;

            while(goal != index.end() && gcount != emi->gtop && goal -> status == "validated"_n && i <= limit) {
                if((goal -> target.amount > 0)) {
                    list_for_emit.push_back(goal->id);
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
     eosio::check(bal->last_recalculated_pool_id == cycle->finish_at_global_pool_id, "Impossible priority enter with not refreshed balance");
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
    auto pool = pools.find(current_pool_id);
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
void unicore::optimize_parameters_of_new_cycle (eosio::name host, eosio::name main_host){
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    auto root_symbol = acc->get_root_symbol();
    
    sincome_index sincome(_me, host.value);
    cycle_index cycles(_me, host.value);
    
    auto last_cycle = cycles.find(acc->current_cycle_num - 1);

    if (last_cycle -> emitted.amount > 0){

        std::string memo = std::string("выдача остатка от сжигания токенов на предыдущем цикле") + std::string(" в DAO ") + host.to_string(); 

        unicore::push_to_dacs_funds(host, last_cycle -> emitted, acc -> root_token_contract, memo);
    }

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
        dp.quants_buy_rate = sp -> base_rate;
        dp.quants_sell_rate = sp -> base_rate;
        dp.quants_convert_rate = sp -> base_rate;

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

void unicore::emplace_first_pools(eosio::name parent_host, eosio::name main_host, eosio::symbol root_symbol, eosio::time_point_sec start_at){
    
    spiral_index spiral(_me, main_host.value);
    auto sp = spiral.find(0);
    eosio::check(sp != spiral.end(), "Protocol is not found. Set parameters at first");
    sincome_index sincome(_me, parent_host.value);
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
    
    sincome.emplace(_me, [&](auto &s){
        s.max = rate -> system_income;
        s.pool_id = available_id;
        s.ahost = main_host;
        s.cycle_num = acc->current_cycle_num;
        s.pool_num = 1;
        s.total = asset(0, root_symbol);
        s.paid_to_sys = asset(0, root_symbol);
        s.paid_to_dacs = asset(0, root_symbol);
        s.paid_to_cfund = asset(0, root_symbol);
        s.paid_to_hfund = asset(0, root_symbol);
        s.paid_to_refs = asset(0, root_symbol);
    }); 

    unicore::cut_tail(available_id, parent_host);
    
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
        s.total = asset(0, root_symbol);
        s.paid_to_sys = asset(0, root_symbol);
        s.paid_to_dacs = asset(0, root_symbol);
        s.paid_to_cfund = asset(0, root_symbol);
        s.paid_to_hfund = asset(0, root_symbol);
        s.paid_to_refs = asset(0, root_symbol);
    }); 
    
    
    unicore::change_bw_trade_graph(parent_host, available_id + 1, acc->current_cycle_num, 2, next_rate->buy_rate, next_next_rate->buy_rate, total_quants, total_quants, "black");
    
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

        unicore::optimize_parameters_of_new_cycle(host, main_host);

        unicore::emplace_first_pools(host, main_host, root_symbol, start_at);

    } else {
        rate_index rates(_me, main_host.value);
        spiral_index spiral(_me, main_host.value);
        
        auto cycle = cycles.find(acc-> current_cycle_num - 1);
        auto rate = rates.find(0);
        auto next_rate = rates.find(1);
        auto sp = spiral.find(0);
        
        pool_index pools(_me, host.value);
        uint64_t available_id = pools.available_primary_key();

        unicore::optimize_parameters_of_new_cycle(host, main_host);
        unicore::emplace_first_pools(host, main_host, root_symbol, start_at);
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

    rate_index rates(_me, main_host.value);
    spiral_index spiral(_me, main_host.value);
    
    auto root_symbol = acc->get_root_symbol();
    auto pool = pools.find(acc -> current_pool_id);
    auto cycle = cycles.find(acc -> current_cycle_num - 1);
    
    auto sp = spiral.find(0);
    
    uint128_t dreserved_quants = 0;
    uint64_t reserved_quants = 0;

    //Если первые два пула не выкуплены, это значит, 
    //не все участники воспользовались приоритетным входом, и пул добавлять не нужно. 

    if (acc -> current_pool_num > 1) {
        auto ratem1 = rates.find(acc-> current_pool_num - 1);
        auto rate = rates.find(acc-> current_pool_num);
        auto ratep1 = rates.find(acc-> current_pool_num + 1);

        eosio::asset host_income = asset(0, root_symbol);
        eosio::asset system_income = asset(0, root_symbol);

        if (rate -> system_income > ratem1 -> system_income) {
            host_income = rate -> system_income - ratem1 -> system_income;
        }

        cycles.modify(cycle, _me, [&](auto &c ){
            c.finish_at_global_pool_id = cycle -> finish_at_global_pool_id + 1;
            c.emitted = asset(0, root_symbol);
        });
        
        eosio::asset quote_amount = unicore::emit(host, host_income, rate -> system_income);
        
        accounts.modify(acc, _me, [&](auto &dp){
           dp.current_pool_num = pool -> pool_num + 1;
           dp.current_pool_id  = pool -> id + 1;
           // dp.quote_amount += quote_amount;

           dp.quants_buy_rate = rate -> buy_rate;
           dp.quants_sell_rate = rate -> sell_rate;
           dp.quants_convert_rate = rate -> convert_rate;


        });

        auto prev_prev_pool = pools.find(acc -> current_pool_id - 2);
        
        dreserved_quants = (prev_prev_pool -> total_quants - prev_prev_pool -> remain_quants ) / sp -> quants_precision  * rate -> sell_rate / rate -> buy_rate * sp -> quants_precision;

        reserved_quants = uint64_t(dreserved_quants);

        eosio::check(pool -> pool_num + 1 <= sp -> pool_limit, "Please, write to help-center for check why you cannot deposit now");
    
        uint64_t pool_id = pools.available_primary_key();
        eosio::time_point_sec expired_at = eosio::time_point_sec (eosio::current_time_point().sec_since_epoch() + sp->pool_timeout);

        pools.emplace(_me, [&](auto &p){
            p.id = pool_id;
            p.ahost = main_host;
            p.total_quants = sp->size_of_pool * sp -> quants_precision;
            p.creserved_quants = 0;
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
        
        accounts.modify(acc, _me, [&](auto &a){
           a.current_pool_num = pool -> pool_num + 1;
           a.current_pool_id  = pool -> id + 1;
           
           a.quants_buy_rate = rate -> buy_rate;
           a.quants_sell_rate = rate -> sell_rate;
           a.quants_convert_rate = rate -> convert_rate;


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
    spiral_index spiral(_me, host.value);
    auto sp = spiral.find(0);
    
    spiral.modify(sp, _me, [&](auto &s){
        s.pool_limit = 22;
    });

    rate_index rates(_me, host.value);

    auto rate = rates.lower_bound(22); // Начинаем с первого элемента, у которого id >= 21

    while (rate != rates.end()) {
        rate = rates.erase(rate); // Удаляем элемент и переходим к следующему
    }


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

    // ------- сделано
      
    // goals_index goals(_me, host.value);
    // auto goal = goals.begin();
    
    // while (goal != goals.end()) {
    //     goal = goals.erase(goal);
    // }

    //  hoststat_index hoststat(_me, host.value);
    //  auto hstat = hoststat.begin();
    //     while (hstat != hoststat.end()) {
    //         hstat = hoststat.erase(hstat);
    //     }


    // // tasks_index tasks(_me, host.value);
    // // auto task = tasks.begin();
    
    // // while (task != tasks.end()) {
    // //     task = tasks.erase(task);
    // // }

    // // reports_index reports(_me, host.value);
    // // auto report = reports.begin();

    // // while (report != reports.end()) {
    // //     report = reports.erase(report);
    // // }

    // // vesting_index vests(_me, host.value);
    // // auto v = vests.begin();
    // // while (v != vests.end()) {
    // //     v = vests.erase(v);
    // // }

    // rate_index rates(_me, host.value);
      
    // auto rate = rates.begin();
    
    // while (rate != rates.end()) {
    //     rate = rates.erase(rate);
    // };

      // if (rate != rates.end()) {
      //   rates.erase(rate);  
      // };

    // account_index accounts(_me, _me.value);
    // auto acc = accounts.find(host.value);
    // // if (acc -> total_shares < sp -> quants_precision){
    //     // accounts.modify(acc, _me, [&](auto &a){
    //     //     a.total_shares = acc -> total_shares / 100;
    //     // });
    // // };
   
    // // pool_index pools(_me, host.value);
    // // auto pool = pools.find(acc -> current_pool_id);
    
    // // pools.modify(pool, _me, [&](auto &p){
    // //     p.pool_expired_at = eosio::time_point_sec(0);
    // // });
    
    // account3_index accounts3(_me, _me.value);
    // auto acc3 = accounts3.find(host.value);
    
    // if (acc3 != accounts3.end()) {
    //     accounts3.erase(acc3);    
    // };
    
    //   if (acc != accounts.end()){
    //     accounts.erase(acc);  
    //   };

    //   market_index market(_me, host.value);
    //   auto itr = market.find(0);
      
    //   if (itr != market.end()) {
    //     market.erase(itr);  
    //   };
      

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
    // // spiral.modify(sp, _me, [&](auto &sp){
    // //     sp.pool_timeout = 2592000;
    // // });

    
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


    //   emission_index emis(_me, host.value);
    //   auto emi = emis.find(host.value);
      
    //   if (emi != emis.end()) {
    //     emis.erase(emi);  
    //   };
    //   // emis.emplace(_me, [&](auto &e){
    //   //   e.host = host;
    //   //   e.percent = 1000000;
    //   //   e.gtop = 0;
    //   //   e.fund = asset(0, _SYM);

    //   // });


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
            a.total_loyality += 1;
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
            ps.liquid_loyality = 1;
            ps.liquid_quants = 0;
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
          pl.loyality_power = 1;
          pl.quants_power = 0;
          pl.available_by_loyality = asset(0, root_symbol);
          pl.available_by_quants = asset(0, root_symbol);
          pl.distributed_to_partners = asset(0, root_symbol);
          pl.updated = false;
        });

        unicore::emplace_first_pools(host, main_host, root_symbol, start_at);
        
        loyality_index loyality(_me, host.value);
        auto lexist = loyality.find(account -> architect.value);

        loyality.emplace(_me, [&](auto &p) {
          p.username = account -> architect;
          p.power = 1;
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
            convert_rate[i] = base_rate;

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

        } else if (i > 0) { 
            sell_rate[i + 1] = buy_rate[i-1] * overlap / ONE_PERCENT;
            
            convert_rate[i] = convert_rate[i - 1] * overlap / ONE_PERCENT;

            client_income[i+1] = uint64_t(sell_rate[i+1]) - uint64_t(buy_rate[i-1]);
            delta[i+1] = i > 1 ? uint64_t(client_income[i+1] * profit_growth / ONE_PERCENT + delta[i]) : client_income[i+1];
            buy_rate[i] = uint64_t(sell_rate[i+1] + delta[i+1]);
            pool_cost[i] = uint64_t(size_of_pool * buy_rate[i]);
            

            if (account -> type == "tokensale"_n) {
                eosio::check(loss_percent == HUNDR_PERCENT, "Only hundr percent losses can be used on this mode");
                payment_to_wins[i] = i > 1 ? payment_to_wins[i - 2] + uint64_t(size_of_pool * (client_income[i]))  : 0;
                live_balance_for_sale[i] = uint64_t(pool_cost[i] - uint64_t(size_of_pool * sell_rate[i]));
            } else {
                payment_to_wins[i] = i > 1 ? uint64_t(size_of_pool * sell_rate[i]) : 0;    
                live_balance_for_sale[i] = uint64_t(pool_cost[i] - uint64_t(payment_to_wins[i]));
            }

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
            // просто не выплачиваем реферальные вознаграждения там, где реферальный поток уменьшается.
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
            r.convert_rate = convert_rate[i];
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
        a.quants_buy_rate = base_rate;
        a.quants_sell_rate = base_rate;
        a.quants_convert_rate = base_rate;

    });

}


void unicore::deposit ( eosio::name username, eosio::name host, eosio::asset amount, eosio::name code, std::string message ){
    
    eosio::check( amount.is_valid(), "Rejected. Invalid quantity" );

    partners2_index users(_partners,_partners.value);
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
    

    print("pool -> remain_quants: ", pool -> remain_quants);
    print("amount: ", amount);
    print("quants: ", quants);


    eosio::check(pool -> remain_quants >= quants, "Not enought Quants in target pool");
    
    unicore::fill_pool(username, host, quants, amount, acc -> current_pool_id);
    
    unicore::add_coredhistory(host, username, acc -> current_pool_id, amount, "deposit", message);
    unicore::add_user_stat("deposit"_n, username, acc->root_token_contract, amount, asset(0, amount.symbol));
    unicore::add_host_stat("deposit"_n, username, host, amount);
    unicore::add_host_stat2("deposit"_n, username, host, amount);
    unicore::add_core_stat("deposit"_n, host, amount);

    // unicore::add_balance(username, amount, acc -> root_token_contract);
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
        // Если меньше 1 кванта - новый пул. 
        
            next_pool(host);
            
        } 

        powerstat_index powerstat(_me, host.value);
        auto pstat = powerstat.find(0);

        if (pstat != powerstat.end()) {
            uint64_t now_secs = eosio::current_time_point().sec_since_epoch();
            uint64_t first_window_start_secs = pstat -> window_open_at.sec_since_epoch();
            uint64_t cycle = (now_secs - first_window_start_secs) / _WINDOW_SECS;

            auto current_pstat = powerstat.find(cycle);
            
            if (current_pstat == powerstat.end()) {

                powerstat.emplace(_me, [&](auto &ps) {
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
                ps.liquid_quants = acc -> total_quants;
                ps.liquid_loyality = acc -> total_loyality;
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
    
    eosio::check(bal -> last_recalculated_pool_id == cycle->finish_at_global_pool_id, "Cannot sell not refreshed balance. Refresh Balance first and try again.");
    balance.modify(bal, _me, [&](auto &b){
        b.status = "onsell"_n;
    });

};




/**
 * @brief      Публичный метод отмены продажи баланса
 * 
 * @param[in]  host  The host
 */

[[eosio::action]] void unicore::cancelsellba(eosio::name host, eosio::name username, uint64_t balance_id){
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

    auto pool = pools.find(bal -> last_recalculated_pool_id - 1);
    auto cycle = cycles.find(pool -> cycle_num - 1);
    
    eosio::check(bal -> last_recalculated_pool_id == cycle->finish_at_global_pool_id, "Cannot buy not refreshed balance. Refresh Balance first and try again.");

    auto user_rate = rates.find(pool -> pool_num - 1);
    
    eosio::check(amount <= bal -> compensator_amount, "Amount for sell should be less or equal compensator amount");
    
    std::vector<eosio::asset> forecasts;
    uint64_t buyer_quants = 0;
    uint64_t buyer_next_quants = 0;
    
    uint64_t buyer_reserved_quants = 0 ;

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
        uint64_t old_reserved_quants = bal -> reserved_quants;
        
        buyer_reserved_quants = amount.amount * old_reserved_quants / old_compensator_amount.amount;
        b.reserved_quants = old_reserved_quants - buyer_reserved_quants;
        uint64_t quants_diff = old_reserved_quants - b.reserved_quants;
        
        if (acc-> type == "tokensale"_n) {
            double double_convert_amount = (double)b.quants_for_sale / (double)bal -> quants_for_sale * (double)bal -> convert_amount.amount;

            b.convert_amount = asset(uint64_t(double_convert_amount), acc-> asset_on_sale.symbol);
            double start_convert_amount = (double)b.quants_for_sale / (double)bal -> quants_for_sale * (double)bal -> start_convert_amount.amount;
            b.start_convert_amount = asset(uint64_t(start_convert_amount), acc-> asset_on_sale.symbol);

        }
        

        double root_percent = (double)b.compensator_amount.amount  / (double)b.purchase_amount.amount * (double)HUNDR_PERCENT - (double)HUNDR_PERCENT; 
        b.root_percent = uint64_t(root_percent);

        action(
            permission_level{ _me, "active"_n },
            _me, "emitquants"_n,
            std::make_tuple( host , bal -> owner, - quants_diff, true) 
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

        b.reserved_quants = buyer_reserved_quants; 

        if (acc-> type == "tokensale"_n) {

            double double_convert_amount = (double)buyer_quants / (double)bal -> quants_for_sale * (double)bal -> convert_amount.amount;
            b.convert_amount = asset(uint64_t(double_convert_amount), acc -> asset_on_sale.symbol);
            double start_convert_amount = (double)buyer_quants / (double)bal -> quants_for_sale * (double)bal -> start_convert_amount.amount;
            b.start_convert_amount = asset(uint64_t(start_convert_amount),acc-> asset_on_sale.symbol);

        }

        double root_percent = (double)b.compensator_amount.amount  / (double)b.purchase_amount.amount * (double)HUNDR_PERCENT - (double)HUNDR_PERCENT; 
        b.root_percent = uint64_t(root_percent);

        b.last_recalculated_pool_id = bal -> last_recalculated_pool_id;
        b.forecasts = forecasts;
        b.withdrawed = asset(0, root_symbol);
        b.ref_amount = asset(0, root_symbol);
        b.dac_amount = asset(0, root_symbol);
        b.cfund_amount = asset(0, root_symbol);
        b.hfund_amount = asset(0, root_symbol);
        b.sys_amount = asset(0, root_symbol);
        b.solded_for = asset(0, root_symbol);
        
        action(
            permission_level{ _me, "active"_n },
            _me, "emitquants"_n,
            std::make_tuple( host , buyer, b.reserved_quants, true) 
        ).send();

        print("BALANCE_ID: ", b.id, ";"); //DO NOT DELETE THIS
    });

};




void unicore::convertbal(eosio::name seller, eosio::name host, uint64_t balance_id){
    require_auth(seller);

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "host is not found");
    eosio::name main_host = acc->get_ahost();
    eosio::check(acc -> type == "tokensale"_n, "Only in token sale mode balance can be converted");

    eosio::check(seller != host, "Host cannot sell balances to self");
    
    cycle_index cycles(_me, host.value);
    pool_index pools(_me, host.value);
    rate_index rates(_me, main_host.value);
    spiral_index spiral(_me, main_host.value);

    auto sp = spiral.find(0);

    auto root_symbol = acc->get_root_symbol();
    balance_index balance(_me, host.value);    
    auto bal = balance.find(balance_id);
    eosio::check(bal != balance.end(), "Balance is not found");
    eosio::check(bal -> status == "onsell"_n, "This balance not on the sale");

    auto pool = pools.find(bal -> last_recalculated_pool_id - 1);
    auto cycle = cycles.find(pool -> cycle_num - 1);
    
    eosio::check(bal -> last_recalculated_pool_id == cycle->finish_at_global_pool_id, "Cannot buy not refreshed balance. Refresh Balance first and try again.");

    //TODO transfer tokens from fund to user
    eosio::asset convert_amount = bal -> convert_amount;
    
    eosio::check(acc -> asset_on_sale >= convert_amount, "Not enought tokens in the sale fund");

    accounts.modify(acc, _me, [&](auto &a) {
        a.asset_on_sale -= convert_amount;
    });

    action(
        permission_level{ _me, "active"_n },
        acc->sale_token_contract, "transfer"_n,
        std::make_tuple( _me, seller, convert_amount, std::string("Convert to sale token")) 
    ).send();

    
    balance.modify(bal, _me, [&](auto &b){
        b.owner = host;
        b.status = "process"_n;
    });

    action(
        permission_level{ _me, "active"_n },
        _me, "emitquants"_n,
        std::make_tuple( host , seller, - bal -> reserved_quants, true) 
    ).send();

    action(
        permission_level{ _me, "active"_n },
        _me, "emitquants"_n,
        std::make_tuple( host , host, bal -> reserved_quants, true) 
    ).send();
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
        forecasts = unicore::calculate_forecast(username, host, quants, pool -> pool_num - 1, amount, amount, true, false);
    else if (acc->current_pool_num == 2)
        forecasts = unicore::calculate_forecast(username, host, quants, pool -> pool_num - 1, amount, amount, true, true);
    else
        forecasts = unicore::calculate_forecast(username, host, quants, pool -> pool_num - 1, amount, amount, true, true);


    pools.modify(pool, _me, [&](auto &p) {
        p.remain_quants = remain_quants;
        uint128_t filled = (pool->total_quants / sp -> quants_precision - p.remain_quants / sp -> quants_precision) * pool->quant_cost.amount;
        p.filled = asset(uint64_t(filled), root_symbol);
        p.remain = p.pool_cost - p.filled;
        p.filled_percent = (pool->total_quants - p.remain_quants)  * HUNDR_PERCENT / pool->total_quants;
        
    });

    uint64_t total_quants = sp->size_of_pool * sp -> quants_precision;

    unicore::change_bw_trade_graph(host, filled_pool_id, pool->cycle_num, pool->pool_num, ratem1->buy_rate, ratep1->buy_rate, total_quants, remain_quants, pool->color);
    
    double reserve_rate = (double)acc -> quants_buy_rate;
    double convert_rate = (double)acc -> quants_convert_rate;
    


    double reserved_quants = double(amount.amount) * (double)sp->quants_precision / double(reserve_rate);
    double converted_amount = double(amount.amount) / double(convert_rate) * pow(10, acc -> asset_on_sale_precision); //потому что курс к 1 полному токену, а не к его сатоши
    
    spiral2_index spiral2(_me, host.value);
    auto sp2 = spiral2.find(0);
    
    uint64_t balance_id = get_global_id("balances"_n);

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
        b.solded_for = asset(0, root_symbol);
        
        b.reserved_quants = uint64_t(reserved_quants);
        
        b.start_reserve_rate = reserve_rate;
        b.start_convert_rate = convert_rate;

        b.convert_amount = asset(uint64_t(converted_amount), acc->asset_on_sale.symbol);
        b.start_convert_amount = b.convert_amount;
        b.convert_percent = 0;
        b.last_recalculated_pool_id = filled_pool_id;
        //зачем это надо? pool -> pool_num == 1 ? filled_pool_id + 1 : filled_pool_id;
        b.withdrawed = asset(0, root_symbol);
        b.ref_amount = asset(0, root_symbol);
        b.dac_amount = asset(0, root_symbol);
        b.cfund_amount = asset(0, root_symbol);
        b.hfund_amount = asset(0, root_symbol);
        b.sys_amount = asset(0, root_symbol);

        if (acc -> type == "tokensale"_n) {

            b.available = asset(0, root_symbol);
            b.compensator_amount = amount;        
            double root_percent = 0; 
            b.root_percent = uint64_t(0);
            b.forecasts = forecasts;

        } else {
            b.available = amount;
            b.compensator_amount = amount;        
            double root_percent = (double)b.compensator_amount.amount  / (double)b.purchase_amount.amount * (double)HUNDR_PERCENT - (double)HUNDR_PERCENT; 
            b.root_percent = uint64_t(root_percent);
            b.forecasts = forecasts;       
        }

        
        action(
            permission_level{ _me, "active"_n },
            _me, "emitquants"_n,
            std::make_tuple( host , username, b.reserved_quants, false) 
        ).send();

    });
    
    print("BALANCE_ID: ", balance_id, ";"); //DO NOT DELETE THIS
}


[[eosio::action]] void unicore::setbalmeta(eosio::name username, eosio::name host, uint64_t balance_id, std::string meta){

    require_auth(username);

    balance_index balance(_me, host.value);
    auto bal = balance.find(balance_id);
    eosio::check(bal != balance.end(), "Balance is not exist or already withdrawed");
    
    balance.modify(bal, username, [&](auto &b){
        b.meta = meta;
    });
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

    if (( partrefresh != 0 )&&(bal -> last_recalculated_pool_id + partrefresh < cycle -> finish_at_global_pool_id)){
        ceiling = bal -> last_recalculated_pool_id + partrefresh;

    } else {
        ceiling = cycle -> finish_at_global_pool_id;
    }
    
    if (bal -> status == "process"_n && bal -> last_recalculated_pool_id <= cycle -> finish_at_global_pool_id)
        for (auto i = bal -> last_recalculated_pool_id + 1;  i <= ceiling; i++){
            std::vector<eosio::asset> forecasts;
            
            auto look_pool = pools.find(i);
            auto purchase_amount = bal-> purchase_amount;
            auto rate = rates.find(look_pool -> pool_num - 1);
                
            if (((acc -> current_pool_num == pool_start -> pool_num ) && (acc -> current_cycle_num == pool_start -> cycle_num)) || \
                ((pool_start -> pool_num < 3) && (pools_in_cycle < 3)) || (has_new_cycle && (pool_start->pool_num == last_pool -> pool_num)))

            { //NOMINAL
                uint64_t old_reserved_quants = bal -> reserved_quants; 
                auto start_rate = rates.find(bal -> pool_num - 1);
                
                balance.modify(bal, payer, [&](auto &b){
                    b.last_recalculated_pool_id = i;
                    double double_reserved_quants = (double)purchase_amount.amount * (double)sp->quants_precision / (double)acc -> quants_buy_rate;

                    b.reserved_quants = uint64_t(double_reserved_quants);
                    
                    int64_t emit_quants = int64_t(old_reserved_quants) - int64_t(b.reserved_quants);

                    action(
                        permission_level{ _me, "active"_n },
                        _me, "emitquants"_n,
                        std::make_tuple( host , username, emit_quants, false) 
                    ).send();


                    auto first_rate = rates.find(bal -> pool_num - 1);
                            
                    if (acc -> type == "tokensale"_n) {
                        double convert_percent = ((double)acc -> quants_convert_rate - (double)start_rate -> convert_rate) / (double)acc -> quants_convert_rate * (double)HUNDR_PERCENT; 
                        b.convert_percent = uint64_t(convert_percent);                                
                    }

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
                        forecasts = unicore::calculate_forecast(username, parent_host, new_quants_for_sale, look_pool -> pool_num - 3, bal->compensator_amount, bal -> purchase_amount, false, false);
                        available = asset(uint64_t((double)new_quants_for_sale * (double)rate -> sell_rate / (double)sp -> quants_precision), root_symbol);                        
                        
                    } else {

                        new_quants_for_sale = bal -> quants_for_sale * prev_win_rate -> sell_rate / prev_win_rate -> buy_rate;
                        new_reduced_quants = new_quants_for_sale * rate -> sell_rate / rate -> buy_rate;
                        
                        if (new_reduced_quants == 0)
                            new_reduced_quants = new_quants_for_sale;
                        forecasts = unicore::calculate_forecast(username, parent_host, new_quants_for_sale, look_pool -> pool_num - 3, bal->compensator_amount, bal -> purchase_amount, false, false);
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
                    
                    // Здесь необходимо взять в расчеты только те границы, при которых системный доход рос, тогда, в setparams можно снять ограничение на это. 
                    if ((middle_rate -> system_income >= start_rate -> system_income) && (finish_rate -> system_income >= middle_rate -> system_income)){
                        
                        eosio::asset incr_amount1 = middle_rate -> system_income - start_rate -> system_income;
                        eosio::asset incr_amount2 = finish_rate -> system_income - middle_rate -> system_income;
                        
                        uint64_t total = incr_amount2.amount / 2;// + incr_amount2.amount; //incr_amount2.amount;// - incr_amount1.amount // //incr_amount2.amount - incr_amount1.amount;
                   
                        double total_ref =   (total * (double)ref_quants / (double)sp -> quants_precision * (double)(acc->referral_percent)) / ((double)HUNDR_PERCENT * (double)sp->size_of_pool);
                        double total_ref_min_sys = total_ref * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
                    
                        asset_ref_amount = asset((uint64_t)total_ref_min_sys, root_symbol);
                         
                    } else {
                        asset_ref_amount = asset(0, root_symbol);
                    }
                        
                        uint64_t old_reserved_quants = bal -> reserved_quants;

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
                            b.last_recalculated_pool_id = i;
                            b.quants_for_sale = new_quants_for_sale;
                            b.next_quants_for_sale = new_reduced_quants;
                            
                            double root_percent = (double)available.amount / (double)bal -> purchase_amount.amount * (double)HUNDR_PERCENT  - (double)HUNDR_PERCENT; 

                            if (acc -> type == "tokensale"_n) {
                                b.available = available - bal -> purchase_amount;
                                root_percent = (double)b.available.amount / (double)bal -> purchase_amount.amount * (double)HUNDR_PERCENT; 
                                
                                
                                double converted_amount = double(available.amount - bal -> withdrawed.amount) / double(start_rate -> convert_rate) * pow(10, acc -> asset_on_sale_precision);                                
                                b.convert_amount = asset(uint64_t(converted_amount), acc->asset_on_sale.symbol);
                                
                                auto first_rate = rates.find(bal -> pool_num - 1);
                                double convert_percent = ((double)acc -> quants_convert_rate - (double)first_rate -> convert_rate) / (double)acc -> quants_convert_rate * (double)HUNDR_PERCENT; 
                                b.convert_percent = uint64_t(convert_percent);                                

                            } else {

                                b.available = available;                                

                            };



                            b.root_percent = (uint64_t)root_percent;
                            b.compensator_amount = available;
                            b.win = true;
                            b.forecasts = forecasts;
                            
                            double double_reserved_quants = double(b.compensator_amount.amount) * (double)sp->quants_precision / double(acc -> quants_buy_rate);
                            
                            b.reserved_quants = uint64_t(double_reserved_quants);
                            
                            int64_t quants_to_emit = int64_t(double(b.reserved_quants) - double(old_reserved_quants));
                            
                            action(
                                permission_level{ _me, "active"_n },
                                _me, "emitquants"_n,
                                std::make_tuple( host , username, quants_to_emit, false) 
                            ).send();
  

                        });

                } else {
                    //LOSS
                    
                    double davailable = (double)bal -> compensator_amount.amount * (double)(HUNDR_PERCENT - sp -> loss_percent) / (double)HUNDR_PERCENT;

                    eosio::asset available = asset(uint64_t(davailable), root_symbol) ;
                    std::vector <eosio::asset> forecasts0 = bal->forecasts;
                    
                    auto distance = look_pool -> pool_num - pool_start -> pool_num;
                    
                    if ( distance > 0 )
                        if (forecasts0.begin() != forecasts0.end())
                            forecasts0.erase(forecasts0.begin());
                    
                    auto middle_rate = rates.find(look_pool -> pool_num - 2);

                    eosio::asset incr_amount2 = (rate -> system_income - middle_rate -> system_income) / 2;

                    uint64_t total = incr_amount2.amount;// + incr_amount2.amount; //incr_amount2.amount;// - incr_amount1.amount // //incr_amount2.amount - incr_amount1.amount;
                    
                    double ref_quants;
                    
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

                    balance.modify(bal, payer, [&](auto &b){
                        b.last_recalculated_pool_id = i;
                        b.win = false;
                        b.available = available;
                        b.forecasts = forecasts0;
                        
                        double root_percent = 0;

                        if (acc -> type == "tokensale"_n) {
                            root_percent = (double)b.available.amount / (double)bal -> purchase_amount.amount * (double)HUNDR_PERCENT; 
                            
                            auto first_rate = rates.find(bal -> pool_num - 1);
                            
                            double converted_amount = double(b.compensator_amount.amount - bal-> withdrawed.amount) / double(first_rate -> convert_rate) *  pow(10, acc -> asset_on_sale_precision);
                            b.convert_amount = asset(uint64_t(converted_amount), acc->asset_on_sale.symbol);

                            double convert_percent = ((double)acc -> quants_convert_rate - (double)first_rate -> convert_rate) / (double)acc -> quants_convert_rate * (double)HUNDR_PERCENT; 
                            b.convert_percent = uint64_t(convert_percent);                                

                        } else {
                            root_percent = (double)bal->available.amount / (double)bal -> purchase_amount.amount * (double)HUNDR_PERCENT  - (double)HUNDR_PERCENT;                         
                        } 

                        b.root_percent = uint64_t(root_percent); 


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

std::vector <eosio::asset> unicore::calculate_forecast(eosio::name username, eosio::name host, uint64_t quants, uint64_t pool_num, eosio::asset available_amount, eosio::asset purchase_amount, bool calculate_first = true, bool calculate_zero = false){
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
        
        forecast0 = loser_amount;

        forcasts.push_back(forecast0);
          
    }     


    if (pool_num + 3 < sp->pool_limit ) {
        
        auto rate1 = rates.find(pool_num + 2);
        auto amount3 = quants * rate1 -> sell_rate / sp -> quants_precision; 
        
        forecast1 = asset(amount3, root_symbol); 
        
        eosio::asset ffractions1 = forecast1;

        if (acc -> type == "tokensale"_n) {
            ffractions1 -= purchase_amount;
        } 

        if (calculate_first)
            forcasts.push_back(ffractions1);

        quants_forecast1 = quants * rate1 -> sell_rate / rate1 -> buy_rate;

    } 
   
   if (pool_num + 4 < sp->pool_limit){
        auto rate15 = rates.find(pool_num + 3);

        forecast15 = asset(forecast1.amount * (HUNDR_PERCENT - sp -> loss_percent) / HUNDR_PERCENT, root_symbol) ;
        
        forcasts.push_back(forecast15);
    }

   
    if (pool_num + 5 < sp->pool_limit){
       
       auto rate2 = rates.find(pool_num + 4);
       double amount5 = (double)quants_forecast1 * (double)rate2 -> sell_rate / (double)sp -> quants_precision;
       forecast2 = asset(amount5, root_symbol);
       quants_forecast2 = quants_forecast1 * rate2 -> sell_rate / rate2 -> buy_rate;

       eosio::asset ffractions2 = forecast2;

        if (acc -> type == "tokensale"_n) {
            ffractions2 -= purchase_amount;
        } 
        

       forcasts.push_back(ffractions2);
    
    } 

     if (pool_num + 6 < sp->pool_limit){
        auto rate25 = rates.find(pool_num + 5);
        forecast25 = asset(forecast2.amount * (HUNDR_PERCENT - sp -> loss_percent) / HUNDR_PERCENT, root_symbol) ;
     
        forcasts.push_back(forecast25);
    }



    if (pool_num + 7 < sp->pool_limit){
        auto rate3 = rates.find(pool_num + 6);
        auto amount7 = (double)quants_forecast2 * (double)rate3 -> sell_rate / (double)sp -> quants_precision;
        forecast3 = asset(amount7, root_symbol);
        quants_forecast3 = quants_forecast2 * rate3 -> sell_rate / rate3 -> buy_rate;
        

        eosio::asset ffractions3 = forecast3;

        if (acc -> type == "tokensale"_n) {
            ffractions3 -= purchase_amount;
        } 
        

        forcasts.push_back(ffractions3);

    }



    if (pool_num + 8 < sp->pool_limit){
        auto rate35 = rates.find(pool_num + 7);
       
        forecast35 = asset(uint64_t((double)forecast3.amount * (double)(HUNDR_PERCENT - sp -> loss_percent) / (double)HUNDR_PERCENT), root_symbol) ;
   
        forcasts.push_back(forecast35);
    }


    if (pool_num + 9 < sp->pool_limit){
        auto rate4 = rates.find(pool_num + 8);
        auto amount9 = (double)quants_forecast3 * (double)rate4 -> sell_rate / (double)sp -> quants_precision;
        forecast4 = asset(amount9, root_symbol);
        quants_forecast4 = quants_forecast3 * rate4 -> sell_rate / rate4 -> buy_rate;


        eosio::asset ffractions4 = forecast4;

        if (acc -> type == "tokensale"_n) {
            ffractions4 -= purchase_amount;
        } 
        

        forcasts.push_back(ffractions4);
    }
    

    if (calculate_first == false){
        if (pool_num + 10 < sp->pool_limit){
            auto rate45 = rates.find(pool_num + 9);
         
            
            forecast45 = asset(uint64_t((double)forecast4.amount * (double)(HUNDR_PERCENT - sp -> loss_percent) / (double)HUNDR_PERCENT), root_symbol) ;
     
            forcasts.push_back(forecast45);
        }


        if (pool_num + 11 < sp->pool_limit){
            auto rate4 = rates.find(pool_num + 10);
            auto amount11 = (double)quants_forecast4 * (double)rate4 -> sell_rate / (double)sp -> quants_precision;
            eosio::asset forecast5 = asset(amount11, root_symbol);
            
            eosio::asset ffractions5 = forecast5;

            if (acc -> type == "tokensale"_n) {
                ffractions5 -= purchase_amount;
            } 
            

            forcasts.push_back(ffractions5);
        }
    }


    return forcasts;
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
    
    eosio::check(bal->last_recalculated_pool_id == cycle->finish_at_global_pool_id, "Cannot withdraw not refreshed balance. Refresh Balance first and try again.");

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

    // eosio::check(bal -> withdrawed == false, "Balance is already withdrawed");

    uint64_t pools_in_cycle = cycle -> finish_at_global_pool_id - cycle -> start_at_global_pool_id + 1;
    /**
     * Номинал
     * Номинал выплачивается если выполняется одно из условий: 
     *  - баланс куплен в одном из первых двух пулов, а текущий пул не выше второго;
     *  - баланс куплен в текущем пуле;
     *  - баланс куплен в последнем из возможных пулов;

     * 
     */

    eosio::check(bal->available.amount > 0, "Cannot withdraw zero balance");
    
    if (((acc -> current_pool_num == pool -> pool_num ) && (acc -> current_cycle_num == pool -> cycle_num)) || \
        ((pool -> pool_num < 3) && (pools_in_cycle < 3)) || (has_new_cycle && (pool->pool_num == last_pool -> pool_num)))

    {
        //в случае acc -> type == "tokensale"_n здесь будет ноль и вывод закрыт

        action(
            permission_level{ _me, "active"_n },
            acc->root_token_contract, "transfer"_n,
            std::make_tuple( _me, username, bal -> available, std::string("User Withdraw")) 
        ).send();

        action(
            permission_level{ _me, "active"_n },
            _me, "emitquants"_n,
            std::make_tuple( host , username, -bal->reserved_quants, false) 
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
        

        //посчитать сколько в квантах нужно срезать
        //из этого посчитать сколько силы нужно срезать
        


        if (amount.amount > 0)
            action(
                permission_level{ _me, "active"_n },
                acc->root_token_contract, "transfer"_n,
                std::make_tuple( _me, username, amount, std::string("User Withdraw")) 
            ).send();
    
        uint64_t quants_from_reserved;
        

        if (bal -> win == true) {

            /**
             * Выигрыш.
             * Инициирует распределение реферальных вознаграждений и выплаты во все фонды.
             */
            
            uint64_t quants_for_sale = bal -> quants_for_sale;
            uint64_t converted_quants = bal -> quants_for_sale * rate -> sell_rate / rate -> buy_rate;
            uint64_t remain_balance_quants = 0;
            eosio::asset convert_amount = bal -> convert_amount;
            uint64_t convert_percent = bal -> convert_percent;

            // uint64_t remain_converted_quants = 0 ;

            if (acc -> type == "tokensale"_n) {

                quants_for_sale = bal -> available.amount * bal -> quants_for_sale / bal -> compensator_amount.amount;
                remain_balance_quants = bal -> quants_for_sale - quants_for_sale;
                converted_quants = quants_for_sale * rate -> sell_rate / rate -> buy_rate;
                
                auto first_rate = rates.find(bal -> pool_num - 1);
                            
                double converted_amount = double(bal -> compensator_amount.amount - (bal -> withdrawed.amount + bal -> available.amount )) / double(first_rate -> convert_rate) *  pow(10, acc -> asset_on_sale_precision);
                convert_amount = asset(uint64_t(converted_amount), acc -> asset_on_sale.symbol);

                double double_convert_percent = ((double)acc -> quants_convert_rate - (double)first_rate -> convert_rate) / (double)acc -> quants_convert_rate * (double)HUNDR_PERCENT; 
                convert_percent = uint64_t(double_convert_percent);                                


            } else {

                //работает всегда, кроме режима токенсейла. Режим токенсейла является накопительным - вывод процентов не уменьшает долю.
                action(
                    permission_level{ _me, "active"_n },
                    _me, "emitquants"_n,
                    std::make_tuple( host , username, - bal -> reserved_quants, false) 
                ).send();
                
            }

            
            /**
             * Возвращаем кванты в пулы
             */
            
            uint64_t remain_quants = std::min(last_pool -> remain_quants + converted_quants, last_pool->total_quants);
            
            pools.modify(last_pool, _me, [&](auto &p){
                p.total_win_withdraw = last_pool -> total_win_withdraw + bal -> available;
                p.remain_quants = remain_quants;

                p.filled = asset( (last_pool->total_quants / sp -> quants_precision - p.remain_quants / sp -> quants_precision) * last_pool->quant_cost.amount, root_symbol);
                p.filled_percent = (last_pool->total_quants - p.remain_quants) * HUNDR_PERCENT / last_pool->total_quants ;
                p.remain = p.pool_cost - p.filled;   
            });

            unicore::change_bw_trade_graph(host, last_pool->id, last_pool->cycle_num, last_pool->pool_num, rate->buy_rate, next_rate->buy_rate, last_pool->total_quants, remain_quants, last_pool -> color);
            unicore::add_coredhistory(host, username, last_pool -> id, bal -> available, "w-withdraw", "");
            
            unicore::add_user_stat("withdraw"_n, username, acc->root_token_contract, bal->purchase_amount, bal->available);
            unicore::add_host_stat("withdraw"_n, username, host, bal->purchase_amount);
            
            //TODO fix emit amount


            if (acc -> type == "tokensale"_n) {
                
                //TODO modify if tokensale mode 
                balance.modify(bal, username, [&](auto &b){
                    b.withdrawed += bal -> available;
                    b.available = asset(0, root_symbol);
                    b.root_percent = 0;
                    b.forecasts = unicore::calculate_forecast(username, host, bal -> quants_for_sale, pool -> pool_num - 1, bal -> compensator_amount, bal -> purchase_amount, true, true);
                    b.convert_percent = convert_percent;
                    b.convert_amount = convert_amount;
                    //TODO add withdrawed
                });

            } else {
                balance.erase(bal);
            }

        } else {
            eosio::check(acc -> type != "tokensale"_n, "Cannot withdraw balance now");

            pools.modify(last_pool, _me, [&](auto &p){
                 p.total_loss_withdraw = last_pool -> total_loss_withdraw + amount;               
            });
            
            unicore::add_user_stat("withdraw"_n, username, acc->root_token_contract, bal->purchase_amount, bal->available);
            unicore::add_host_stat("withdraw"_n, username, host, bal->purchase_amount);
        
            
            //JUST WITHDRAW LOSE AMOUNT
            unicore::add_coredhistory(host, username, last_pool -> id, amount, "l-withdraw", "");  

            //TODO! Выдавать баллы лояльности на сумму "проигрыша"
            // unicore::add_vbalance(username, host, amount);    

            action(
                permission_level{ _me, "active"_n },
                _me, "emitquants"_n,
                std::make_tuple( host , username, -bal->reserved_quants, false) 
            ).send();

            balance.erase(bal);
        }
        
        
        // check_good_status(host, username, bal -> purchase_amount - bal -> available);

        
    
    }
};


    
    void unicore::spread_to_funds(eosio::name code, eosio::name host, eosio::asset total, std::string comment) {
        // gpercents_index gpercents(_me, _me.value);
        // auto syspercent = gpercents.find("system"_n.value);
        // eosio::check(syspercent != gpercents.end(), "Contract is not active");

        // account_index accounts(_me, _me.value);
        // auto acc = accounts.find(host.value);
        // auto main_host = acc->get_ahost();
        // auto root_symbol = acc->get_root_symbol();
        
        // eosio::check(root_symbol == total.symbol, "Wrong symbol for distribution");
        // eosio::check(code == acc->root_token_contract, "Wrong root token for this host");
    
        // eosio::asset total_dac_asset;
        // eosio::asset total_cfund_asset;
        // eosio::asset total_hfund_asset;
        // eosio::asset total_sys_asset;
        

        // spiral_index spiral(_me, host.value);
        // auto sp = spiral.find(0);

        // rate_index rates(_me, host.value);
        // auto start_rate = rates.find(acc -> current_pool_num - 1);
        // auto middle_rate = rates.find(acc -> current_pool_num);
        // auto finish_rate = rates.find(acc -> current_pool_num + 1);

        // // uint64_t total = uint64_t((double)quants * (double)start_rate -> buy_rate) / sp -> quants_precision;
        // // print("buy_rate", start_rate -> buy_rate);


        // double total_ref = ((double)total.amount * (double)(acc->referral_percent)) / ((double)HUNDR_PERCENT);
        // double total_ref_min_sys = total_ref * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
        // double total_ref_sys = total_ref * syspercent -> value / HUNDR_PERCENT;


        // double total_dac = (double)total.amount * (double)(acc->dacs_percent) / (double)HUNDR_PERCENT;
        // double total_dac_min_sys = total_dac * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
        // double total_dac_sys = total_dac * syspercent -> value/ HUNDR_PERCENT;
    

        // double total_cfund = (double)total.amount * (double)(acc->cfund_percent) / ((double)HUNDR_PERCENT);
        // double total_cfund_min_sys = total_cfund * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
        // double total_cfund_sys = total_cfund * syspercent -> value/ HUNDR_PERCENT;


        // double total_hfund = (double)total.amount * (double)(acc->hfund_percent) / (double)HUNDR_PERCENT;  
        // double total_hfund_min_sys = total_hfund * (HUNDR_PERCENT - syspercent -> value) / HUNDR_PERCENT;
        // double total_hfund_sys = total_hfund * syspercent -> value/ HUNDR_PERCENT;
    

        // eosio::asset asset_ref_amount = asset((uint64_t)total_ref_min_sys, root_symbol);

        
        // total_dac_asset = asset((uint64_t)total_dac_min_sys, root_symbol);
        // total_cfund_asset = asset((uint64_t)total_cfund_min_sys, root_symbol);
        // total_hfund_asset = asset((uint64_t)total_hfund_min_sys, root_symbol);
        
        
        // double total_sys = total_dac_sys + total_cfund_sys + total_hfund_sys + total_ref_sys;
        // total_sys_asset = asset((uint64_t)total_sys, root_symbol);


        // //REFS 
        
        // if (total_dac_asset.amount > 0) {
        //     unicore::push_to_dacs_funds(host, total_dac_asset, code, "свободное входящие поступление");
        // }

        // if (total_hfund_asset.amount > 0) {
        //     powerstat_index powerstats(_me, host.value);
        //     auto first_window = powerstats.find(0);

        //     if (first_window != powerstats.end()) {

        //         uint64_t now_secs = eosio::current_time_point().sec_since_epoch();
        //         uint64_t first_window_start_secs = first_window -> window_open_at.sec_since_epoch();
        //         uint64_t cycle = (now_secs - first_window_start_secs) / _WINDOW_SECS;

        //         auto current_window = powerstats.find(cycle);
                
        //         if (current_window != powerstats.end()) {
        //             powerstats.modify(current_window, _me, [&](auto &ps) {
        //                 ps.total_available += total_hfund_asset;
        //                 ps.total_remain += total_hfund_asset;
        //                 ps.total_partners_available += asset_ref_amount;
        //             });
               
                  
        //         } else {
        //           //emplace window if something happen with auto-refresh state

        //           powerstats.emplace(_me, [&](auto &ps) {
        //               ps.id = cycle;
        //               ps.window_open_at = eosio::time_point_sec(first_window_start_secs + _WINDOW_SECS * cycle);
        //               ps.window_closed_at = eosio::time_point_sec(first_window_start_secs + _WINDOW_SECS * (cycle + 1));
        //               ps.liquid_power = acc -> total_shares;
        //               ps.total_available = total_hfund_asset;
        //               ps.total_remain = total_hfund_asset;
        //               ps.total_distributed = asset(0, root_symbol);
        //               ps.total_partners_available = asset_ref_amount;
        //               ps.total_partners_distributed = asset(0, root_symbol);
        //           });

        //         }
        //     } else {

        //         powerstats.emplace(_me, [&](auto &ps){
        //             ps.id = 0;
        //             ps.window_open_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
        //             ps.window_closed_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + _WINDOW_SECS);
        //             ps.liquid_power = acc -> total_shares;
        //             ps.total_available = total_hfund_asset;
        //             ps.total_remain = total_hfund_asset;
        //             ps.total_distributed = asset(0, root_symbol);
        //             ps.total_partners_available = asset_ref_amount;
        //             ps.total_partners_distributed = asset(0, root_symbol);
        //         }); 
                
        //     };
        //   };
        
        // if (total_cfund_asset.amount > 0) {
        //     emission_index emis(_me, host.value);
        //     auto emi = emis.find(host.value);
        //     eosio::check(emi != emis.end(), "Emission pool for current host is not found");
            
        //     emis.modify(emi, _me, [&](auto &e) {
        //         e.fund += total_cfund_asset;
        //     });
        // }
        
        // if (total_sys_asset.amount > 0) {
        //     // Выплаты на системный аккаунт сообщества. 
        //     action(
        //         permission_level{ _me, "active"_n },
        //         "eosio"_n, "inprodincome"_n,
        //         std::make_tuple( acc -> root_token_contract, total_sys_asset) 
        //     ).send();
        // };


    }

    void unicore::spread_to_refs(eosio::name host, eosio::name username, eosio::asset spread_amount, eosio::asset from_amount, eosio::name token_contract){
        partners2_index refs(_partners, _partners.value);
        auto ref = refs.find(username.value);

        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        
        eosio::check(acc -> root_token_contract == token_contract, "Wrong token contract");

        uint64_t usdtwithdraw = unicore::getcondition(host, "usdtwithdraw");


        
        eosio::name referer;
        uint8_t count = 1;
        bool user_is_member = true;

        if (acc -> type == "tokensale"_n){
            uint64_t payformem = unicore::getcondition(host, "payformlm");
            bool paid_membership = payformem == 0 ? false : true;
            if (paid_membership == true) {
                uint64_t core_product_id = unicore::getcondition(host, "coreproduct");
               
                if (core_product_id > 0) {
                    user_is_member = !unicore::get_product_expiration(host, username, core_product_id);  
                };
            };

        };

        if ((ref != refs.end()) && ((ref->referer).value != 0) && user_is_member) {
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
                    
                    std::string memo = std::string("выплата реферальных вознаграждений с ") + std::to_string(count) 
                        + " уровня от партнёра " + username.to_string() 
                        + std::string(" в DAO ") + host.to_string(); 

                    unicore::push_to_dacs_funds(referer, to_ref_amount, acc -> root_token_contract, memo);


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
             * Все неиспользуемые вознаграждения с вышестояющих уровней отправляются на пользователя  ? 
             * Все неиспользуемые вознаграждения с вышестояющих уровней отправляются на компании      ? 
             */
            eosio::asset back_to_host = spread_amount - paid;
            
            if (back_to_host.amount > 0) {

                std::string memo = std::string("возврат не использованных реферальных вознаграждений")
                    + std::string(" в DAO ") + host.to_string(); 

                unicore::push_to_dacs_funds(host, back_to_host, acc -> root_token_contract, memo);
                
            }


            
        } else {
            /**
             * Если рефералов у пользователя нет, то переводим все реферальные средства пользователю.
             * * Если рефералов у пользователя нет, то переводим все реферальные средства компании.
             */
            if (spread_amount.amount > 0) {
                std::string memo = std::string("возврат не использованных реферальных вознаграждений")
                    + std::string(" в DAO ") + host.to_string(); 

                unicore::push_to_dacs_funds(host, spread_amount, acc -> root_token_contract, memo);
                    
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




    void unicore::push_to_dacs_funds(eosio::name host, eosio::asset amount, eosio::name contract, std::string memo) {

        dacsincome_index dacsincome(_me, host.value);
        
        dacsincome.emplace(_me, [&](auto &l){
            l.id = unicore::get_global_id("dacslist"_n);
            l.amount = amount;
            l.contract = contract;
            l.memo = memo;
        });

    };


    std::tuple <eosio::asset, eosio::name, std::string> unicore::push_from_dacs_funds(eosio::name host) {
        dacsincome_index dacsincome(_me, host.value);

        auto min_id_dac_it = dacsincome.begin();

        if (min_id_dac_it != dacsincome.end()) {
            auto min_id_dac_amount = min_id_dac_it->amount;
            auto min_id_dac_contract = min_id_dac_it->contract;
            auto memo = min_id_dac_it -> memo;
            dacsincome.erase(min_id_dac_it);

            return std::tuple(min_id_dac_amount, min_id_dac_contract, memo);    

        } else {

            account_index accounts(_me, _me.value);

            return std::tuple(asset(0, _SYM), ""_n, std::string(""));    

        }

        
    }


    [[eosio::action]] void unicore::spreadlist(eosio::name host) {
        require_auth(host);
        spread_to_dacs(host);
    }


    void unicore::spread_to_dacs(eosio::name host) {

        auto [amount, contract, memo] = unicore::push_from_dacs_funds(host);
        
        if (amount.amount > 0){

            dacs_index dacs(_me, host.value);
            account_index accounts(_me, _me.value);
            auto acc = accounts.find(host.value);

            if (acc != accounts.end()) {

                auto root_symbol = acc->get_root_symbol();
                eosio::check(contract == acc -> root_token_contract, "Wrong token contract for spread");
                eosio::check(amount.symbol == root_symbol, "System error on spead to dacs");

                auto dac = dacs.begin();
                
                if (dac != dacs.end()){
                    while(dac != dacs.end()) {
                
                        eosio::asset amount_for_dac = asset(amount.amount * dac -> weight / acc-> total_dacs_weight, root_symbol);
                        
                        action(
                            permission_level{ _me, "active"_n },
                            contract, "transfer"_n,
                            std::make_tuple( _me, dac -> dac, amount_for_dac, memo) 
                        ).send();
                        
                        dac++;
                        
                    }
                } else {
                    //transfer to host if dac not exist
                    action(
                        permission_level{ _me, "active"_n },
                        contract, "transfer"_n,
                        std::make_tuple( _me, host, amount, memo) 
                    ).send();
                }
            } else {

                action(
                    permission_level{ _me, "active"_n },
                    contract, "transfer"_n,
                    std::make_tuple( _me, host, amount, memo) 
                ).send();

            }
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
    spiral_index spiral(_me, host.value);
    auto sp = spiral.find(0);
    
    
    eosio::check(acc != accounts.end(), "Host is not found");
    eosio::check(acc -> type == "tin"_n, "Cannot change mode after first change");
    
    if (mode == "tokensale"_n) {
        eosio::check(acc -> parameters_setted == false, "This mode can be setted only before parameters");
    };
    

    accounts.modify(acc, host, [&](auto &h){
        h.type = mode;
    });
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

    eosio::asset core_amount = quantity;
    
    uint128_t dquants = (uint128_t)core_amount.amount * (uint128_t)sp->quants_precision / (uint128_t)rate->buy_rate;
    uint64_t quants = dquants;
   
    uint64_t quants_for_buy;

    eosio::asset remain_asset = asset(0, root_symbol);
    uint64_t remain_quants = 0;


    cycle_index cycles(_me, host.value);    
    auto cycle = cycles.find(acc->current_cycle_num - 1);

    cycles.modify(cycle, _me, [&](auto &c){
        c.emitted += quantity;
    });


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
    });
    

    unicore::add_coredhistory(host, username, pool -> id, quantity, "burn", "");    
    unicore::change_bw_trade_graph(host, pool -> id, pool -> cycle_num, pool -> pool_num, rate -> buy_rate, next_rate -> buy_rate, pool -> total_quants, pool -> remain_quants, pool -> color);

    if (acc -> type == "tin"_n){
        unicore::add_vbalance(username, host, quantity);
    }   

    if (remain_asset.amount > 0) {

        std::string memo = std::string("остаток от сжигания токенов") + std::string(" в DAO ") + host.to_string(); 

        unicore::push_to_dacs_funds(host, remain_asset, acc -> root_token_contract, memo);
    }
    
    unicore::refresh_state(host);
    
}


/**
 * @brief Публичный метод сжигания квантов по текущему курсу из числа квантов раунда.
 *
*/

void unicore::subscribe_action(eosio::name username, eosio::name host, eosio::asset quantity, eosio::name code, eosio::name status){
    
    // account_index accounts(_me, _me.value);
    // auto acc = accounts.find(host.value);
    
    // eosio::check(acc != accounts.end(), "Host is not found");
    
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

    // eosio::asset ref_amount = quantity * acc -> referral_percent / HUNDR_PERCENT;
    // eosio::asset core_amount = quantity - ref_amount; 
    
    // if (ref_amount.amount > 0) {
    //     unicore::spread_to_refs(host, username, ref_amount, quantity, acc -> root_token_contract);
    // };

    // uint128_t dquants = (uint128_t)core_amount.amount * (uint128_t)sp->quants_precision / (uint128_t)rate->buy_rate;
    // uint64_t quants = dquants;
   

    // //TODO make recursive and return summ of buys
    // // eosio::check(pool->remain_quants >= quants, "Not enought quants in target pool");

    // uint64_t quants_for_buy;

    // eosio::asset remain_asset = asset(0, root_symbol);
    // uint64_t remain_quants = 0;

    // if (pool->remain_quants < quants) {

    //     quants_for_buy = pool->remain_quants;
    //     remain_asset = quantity - pool->remain;
    //     remain_quants = quants - pool->remain_quants;
        
    // } else {
        
    //     quants_for_buy = quants;
    
    // };

    
    // pools.modify(pool, _me, [&](auto &p) {
    //     p.remain_quants = pool -> remain_quants - quants_for_buy;
    //     p.filled = asset(( pool -> total_quants - p.remain_quants) / sp -> quants_precision * pool->quant_cost.amount, root_symbol);
    //     p.filled_percent = (pool -> total_quants - p.remain_quants) * HUNDR_PERCENT / pool->total_quants;
    //     p.remain = p.pool_cost - p.filled;
    //     // p.creserved_quants += quants_for_buy; 
    // });
    

    // unicore::add_coredhistory(host, username, pool -> id, quantity, "burn", "");    
    // unicore::change_bw_trade_graph(host, pool -> id, pool -> cycle_num, pool -> pool_num, rate -> buy_rate, next_rate -> buy_rate, pool -> total_quants, pool -> remain_quants, pool -> color);
    
    // if (host != "core"_n)
    //     unicore::check_subscribe(host, username, quantity, status);
    
    // if (remain_asset.amount > 0) {
    
    //     unicore::push_to_dacs_funds(host, remain_asset, acc -> root_token_contract, "возврат не использованных реферальных вознаграждений");
                
    // }
    

    // // double power = (double)quantity.amount / (double)acc -> quants_buy_rate * (double)sp->quants_precision;
    // // int64_t user_power = int64_t(power);

    // // action(
    // //     permission_level{ _me, "active"_n },
    // //     _me, "emitquants"_n,
    // //     std::make_tuple( host , username, user_power, false) 
    // // ).send();

    // unicore::refresh_state(host);
    
}


