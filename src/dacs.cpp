

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


