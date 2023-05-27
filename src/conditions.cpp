using namespace eosio;

    //add to AWL

    void unicore::checkminpwr(eosio::name host, eosio::name username){
        power3_index upower(_me, host.value);
        auto hp = upower.find(username.value);

        if (hp != upower.end()){
            if (unicore::checkcondition(host, "minpower", hp->staked + hp->with_badges))
            {
                unicore::addtohostwl(host, username);
            } else {
                unicore::rmfromhostwl(host, username);
            }

        }
    }

    uint64_t unicore::get_status_level(eosio::name host, eosio::name username) {
        
        corepartners_index cpartners(_me, host.value);
        auto partner = cpartners.find(username.value);

        uint64_t level = 0;
        eosio::name status = "cat"_n;

        if (partner != cpartners.end()) {
            status = partner -> status;
        };

        if (status == "koala"_n)        {
            level = 1;
        } else if (status == "panda"_n) {
            level = 2;
        } else if (status == "wolf"_n)  {
            level = 3;
        } else if (status == "tiger"_n) {
            level = 4;
        } else if (status == "leo"_n)   {
            level = 5;
        } else if (status == "bear"_n)  {
            level = 6;
        } else if (status == "dragon"_n) {
            level = 7;
        }

        return level;  
    }

    bool unicore::get_status_expiration(eosio::name host, eosio::name username) {
        
        corepartners_index cpartners(_me, host.value);
        auto partner = cpartners.find(username.value);
        
        eosio::time_point_sec expiration = eosio::time_point_sec(0);

        if (partner != cpartners.end()) {
            expiration = partner -> expiration;
        };

        //
        bool is_expired = eosio::current_time_point().sec_since_epoch() >= expiration.sec_since_epoch();

        return is_expired;  
    }




    void unicore::check_subscribe(eosio::name host, eosio::name username, eosio::asset amount, eosio::name status) {
        uint64_t adviser_amount = 30000;
        uint64_t assistant_amount = 1000000;

        eosio::check(status == "adviser"_n || status == "assistant"_n, "wrong status. [adviser, assistant]");
        uint64_t minburn = status == "adviser"_n ? adviser_amount : assistant_amount;

        uint64_t times = amount.amount / minburn;        
        
        if (times > 0) {
            
            corepartners_index cpartners(_me, host.value);
            auto partner = cpartners.find(username.value);
            
            if (partner == cpartners.end()) {
                
                cpartners.emplace(_me, [&](auto &p) {
                    p.username = username;
                    p.status = status;
                    p.join_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
                    p.expiration = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + times * 30 * 86400); //
                });

            } else {

                cpartners.modify(partner, _me, [&](auto &p) {

                    if (partner -> status == status){
                        p.expiration = eosio::time_point_sec(partner->expiration.sec_since_epoch() + times * 30 * 86400); 
                    } else {
                        p.expiration = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + times * 30 * 86400);
                    }

                    p.status = status;
                });

            }
        } else eosio::check(false, "Not enough amount for buy current status");
        
    }


    void unicore::check_status(eosio::name host, eosio::name username, eosio::asset amount, eosio::name status){
        uint64_t minburn = 100000; //10 FLOWER //unicore::getcondition(host, "minburn"); 
        uint64_t threemburn = minburn * 0.9;    //unicore::getcondition(host, "threemburn");
        uint64_t sixmburn = minburn * 0.8;//unicore::getcondition(host, "sixmburn");
        uint64_t ninemburn = minburn * 0.7;//unicore::getcondition(host, "ninemburn");
        uint64_t twelvemburn = minburn * 0.5;//unicore::getcondition(host, "twelvemburn");

        uint64_t burnperiod = 30; //days //unicore::getcondition(host, "burnperiod");
        
        uint64_t times = 0;
        
        uint64_t devider;

        if (status == "koala"_n)        {
            devider = 1;
        } else if (status == "panda"_n) {
            devider = 2;
        } else if (status == "wolf"_n)  {
            devider = 3;
        } else if (status == "tiger"_n) {
            devider = 4;
        } else if (status == "leo"_n)   {
            devider = 5;
        } else if (status == "bear"_n)  {
            devider = 6;
        } else if (status == "dragon"_n) {
            devider = 7;
        } else eosio::check(false, "wrong status. [koala, panda, wolf, tiger, leo, bear, dragon]");


        if (twelvemburn > 0 && amount.amount >= twelvemburn * 12) {
            times = amount.amount / twelvemburn / devider;
        } else if (ninemburn > 0 && amount.amount >= ninemburn * 9) {
            times = amount.amount / ninemburn / devider;
        } else if (sixmburn > 0 && amount.amount >= sixmburn * 6) {
            times = amount.amount / sixmburn / devider;
        } else if (threemburn > 0 && amount.amount >= threemburn * 3) {
            times = amount.amount / threemburn / devider;
        } else if (minburn > 0 && amount.amount >= minburn) {
            times = amount.amount / minburn / devider;
        }
        
        if (times > 0) {
            
            corepartners_index cpartners(_me, host.value);
            auto partner = cpartners.find(username.value);
            
            
            if (partner == cpartners.end()) {
                
                cpartners.emplace(_me, [&](auto &p) {
                    p.username = username;
                    p.status = status;
                    p.total_good = amount;
                    p.join_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
                    p.expiration = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + times * burnperiod * 86400); //
                });

            } else {

                cpartners.modify(partner, _me, [&](auto &p) {
                    p.status = status;
                    p.expiration = eosio::time_point_sec(partner->expiration.sec_since_epoch() + times * burnperiod * 86400); 
                    p.total_good += amount;
                });

            }
        } else eosio::check(false, "Not enough amount for buy current status");
        
    }

    void unicore::rmfromhostwl(eosio::name host, eosio::name username){
        corepartners_index partners(_me, host.value);

        auto partner = partners.find(username.value);
        if (partner != partners.end()) {
            partners.erase(partner);
        }
    }    

    void unicore::addtohostwl(eosio::name host, eosio::name username){
        corepartners_index partners(_me, host.value);

        auto partner = partners.find(username.value);
        if (partner == partners.end()) {
            partners.emplace(_me, [&](auto &p){
                p.username = username;
                p.status = "partner"_n;
            });
        }
    }    

    bool unicore::checkcondition(eosio::name host, eosio::string key, uint64_t value){
        conditions_index conditions(_me, host.value);
        eosio::name keyname = name(key);

        auto condition = conditions.find(keyname.value);

        if (condition != conditions.end()){
            if (value >= condition -> value && condition -> value != 0)
                return true;
            else return false;
        } 

        return false;
    }

    uint64_t unicore::getcondition(eosio::name host, eosio::string key){
        conditions_index conditions(_me, host.value);
        eosio::name keyname = name(key);

        auto condition = conditions.find(keyname.value);
        uint64_t result = 0;

        if (condition != conditions.end()){
            result = condition -> value;
        };

        return result;
    }


    [[eosio::action]] void unicore::checkstatus(eosio::name host, eosio::name username){
        require_auth ("eosio"_n);
        // corepartners_index cpartners(_me, host.value);
        // auto partner = cpartners.find(username.value);

        // if (partner != cpartners.end() && partner -> expiration.sec_since_epoch() <= eosio::current_time_point().sec_since_epoch())
        //     cpartners.erase(partner);

    }

    [[eosio::action]] void unicore::setcondition(eosio::name host, eosio::string key, uint64_t value){
        require_auth (host);

        conditions_index conditions(_me, host.value);
        eosio::name keyname = name(key);

        auto condition = conditions.find(keyname.value);

        if (condition == conditions.end()){
            
            conditions.emplace(host, [&](auto &c){
                c.key = keyname.value;
                c.key_string = key;
                c.value = value;
            });

        } else {

            conditions.modify(condition, host, [&](auto &c) {
                c.value = value;
            });

        }
    }

    [[eosio::action]] void unicore::rmcondition(eosio::name host, uint64_t key){
        require_auth (host);

        conditions_index conditions(_me, host.value);
        eosio::name keyname = name(key);

        auto condition = conditions.find(keyname.value);

        if (condition != conditions.end()){
            conditions.erase(condition);
        }
    }
