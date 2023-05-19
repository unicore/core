

using namespace eosio;
/**
 * @brief      Структура хоста
 */


    /**
     * @brief      Returns a string representation of an asset.
     *
     * @param[in]  val   The value
     *
     * @return     String representation of the asset.
     */
    std::string unicore::symbol_to_string(eosio::asset val) const
    {
        uint64_t v = val.symbol.code().raw();
        v >>= 8;
        string result;
        while (v > 0) {
                char c = static_cast<char>(v & 0xFF);
                result += c;
                v >>= 8;
        }
        return result;
    }

    /**
     * @brief      Sets the architect.
     * Устанавливает архитектора сообщества, обладающего полномочиями специальных действий. 
     * 
     * @param[in]  op    The new value
     */
    [[eosio::action]] void unicore::setarch(name host, name architect){
        
        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Host is not found");
        
        eosio::check(has_auth(_me) || has_auth(host), "missing required authority");
        
        accounts.modify(acc, _me, [&](auto &a){
            a.architect = architect;
        });

    };

    /**
     * @brief      Установка компенсатора рисков
     * Устанавливает компенсатор риска для системного жетона
     * 
     * @param[in]  op    The new value
     */
    [[eosio::action]] void unicore::compensator(eosio::name host, uint64_t compensator_percent){
        require_auth("eosio"_n);

        // account_index accounts(_me, _me.value);
        // auto acc = accounts.find(host.value);
        // eosio::check(acc != accounts.end(), "Host is not found");
        
        // eosio::name main_host = acc->get_ahost();
        // spiral_index spiral(_me, main_host.value);
        // auto sp = spiral.find(0);
        // spiral2_index spiral2(_me, main_host.value);
        // auto sp2 = spiral2.find(0);
    
        // eosio::check(acc->root_token_contract == "eosio.token"_n, "Only eosio.token contract can be used for compesation");
        // eosio::check(acc->root_token.symbol == _SYM, "Only core symbol can be used for compensation.");

        // eosio::check(host == _core_host, "Only core host can access compensator now");
    
        // eosio::check(sp -> overlap <= 10500, "Overlap should be not more then 5% for compensator mode");
        // eosio::check(compensator_percent <= sp -> loss_percent, "Compensator Percent must be greater then 0 (0%) and less or equal 10000 (100%)");
    
        // if (sp2 == spiral2.end()) {
            
        //     spiral2.emplace(_me, [&](auto &s) {
        //         s.id = 0;
        //         s.compensator_percent = compensator_percent;
        //     });

        // } else {

        //     spiral2.modify(sp2, _me, [&](auto &s) {
        //         s.compensator_percent = compensator_percent;
        //     });
        // }
        

    };


    [[eosio::action]] void unicore::addvac(uint64_t id, bool is_edit, eosio::name creator, eosio::name host, eosio::name limit_type, eosio::asset income_limit, uint64_t weight, std::string title, std::string descriptor) {
        require_auth(creator);

        account_index accounts(_me, _me.value);
        vacs_index vacs(_me, host.value);

        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Host is not found");
        

        auto root_symbol = acc->get_root_symbol();
        eosio::check(income_limit.symbol == root_symbol, "Wrong root symbol");

        if (is_edit == true) {
            
            auto vac = vacs.find(id);
            eosio::check(vac != vacs.end(), "Vac is not found");
            
            vacs.modify(vac, creator, [&](auto &v){
                v.description = descriptor;
                v.role = title;
            });

        } else {

            vacs.emplace(creator, [&](auto &d){
                d.id = vacs.available_primary_key();
                d.creator = creator;
                d.limit_type = limit_type;
                d.income_limit = income_limit;
                d.weight = weight;
                d.approved = creator == host ? true : false;
                d.role = title;
                d.description = descriptor;
            });

        };

        

    };

    [[eosio::action]] void unicore::addvprop(uint64_t id, bool is_edit, eosio::name creator, eosio::name host, uint64_t vac_id, uint64_t weight, std::string why_me, std::string contacts) {
        require_auth(creator);

        vproposal_index vprops(_me, host.value);
        vacs_index vacs(_me, host.value);
        auto vac = vacs.find(vac_id);
        
        eosio::check(vac != vacs.end(), "VAC is not found");
        eosio::check(vac->closed == false, "VAC is already closed");

        if (is_edit == false){
            vacs.modify(vac, creator, [&](auto &v){
                v.proposals = vac -> proposals + 1;
            });

            vprops.emplace(creator, [&](auto &d){
                d.id = vprops.available_primary_key();
                d.vac_id = vac_id;
                d.creator = creator;
                d.weight = weight;
                d.why_me = why_me;
                d.contacts = contacts;
            });
        
        } else {
            auto vprop = vprops.find(id);
            eosio::check(vprop != vprops.end(), "Proposal is not found");

            vprops.modify(vprop, creator, [&](auto &d){
                d.why_me = why_me;
                d.contacts = contacts;
            });

        }
    };




    [[eosio::action]] void unicore::rmvac(eosio::name host, uint64_t id) {
        require_auth(host);
        vacs_index vacs(_me, host.value);

        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Host is not found");
        
        auto vac = vacs.find(id);

        eosio::check(vac != vacs.end(), "VAC is not found");
        eosio::check(vac -> proposals == 0, "VAC's proposals should be a zero" );

        vacs.erase(vac);
    };


    [[eosio::action]] void unicore::approvevac(eosio::name host, uint64_t vac_id) {
        require_auth(host);
        vacs_index vacs(_me, host.value);

        auto vac = vacs.find(vac_id);

        eosio::check(vac != vacs.end(), "VAC is not found");

        vacs.modify(vac, host, [&](auto &v){
            v.approved = true;
        });

    };

    [[eosio::action]] void unicore::apprvprop(eosio::name host, uint64_t vprop_id) {
        require_auth(host);

        vproposal_index vprops(_me, host.value);

        auto vprop = vprops.find(vprop_id);
        eosio::check(vprop != vprops.end(), "VAC of the proposal is not found");

        vacs_index vacs(_me, host.value);
        
        auto vac = vacs.find(vprop -> vac_id);

        eosio::check(vac != vacs.end(), "VAC is not found");
        eosio::check(vac -> closed == false, "VAC is already closed" );

        unicore::adddac(vprop -> creator, host, vprop -> weight, vac -> limit_type, vac -> income_limit, vac -> role, vac -> description);

        vacs.modify(vac, host, [&](auto &v) {
            v.closed = true;
        });


        vprops.modify(vprop, host, [&](auto &v){
            v.closed = true;
        });

    };

    [[eosio::action]] void unicore::rmvprop(eosio::name host, uint64_t vprop_id) {
        
        vproposal_index vprops(_me, host.value);

        auto vprop = vprops.find(vprop_id);
        eosio::check(vprop != vprops.end(), "Proposal is not found");

        eosio::check(has_auth(host) || has_auth(vprop -> creator), "missing required authority");
        
        vacs_index vacs(_me, host.value);
        
        auto vac = vacs.find(vprop -> vac_id);

        eosio::check(vac != vacs.end(), "VAC is not found");

        vacs.modify(vac, host, [&](auto &v) {
            v.proposals = vac->proposals - 1;
        });

        vprops.erase(vprop);

    };


    [[eosio::action]] void unicore::setahost( eosio::name host, eosio::name ahostname){
        eosio::name payer;

        conditions_index conditions(_me, host.value);
        eosio::name keyname = name("condjoinhost");
        auto condition = conditions.find(keyname.value);
        

        if (condition != conditions.end() && condition -> value == "architect"_n.value){
            payer = host;
            require_auth(host); 
        } else {
            eosio::check(has_auth(host) || has_auth(ahostname), "missing required authority");
            payer = has_auth(host) ? host : ahostname;
        }
                
        ahosts_index coreahosts(_me, _me.value);
        auto corehost = coreahosts.find(host.value);

        eosio::check(corehost != coreahosts.end(), "Core host is not found");
        // eosio::check(corehost -> type == "platform"_n, "Host is not a platform");

        ahosts_index ahosts(_me, host.value);
        auto ahost = ahosts.find(ahostname.value);

        eosio::check(ahost == ahosts.end(), "Host is already exist in a platform");
        print("in here");

        auto coreahost = coreahosts.find(ahostname.value);
        eosio::check(coreahost != coreahosts.end(), "Core ahost is not found");

        ahosts.emplace(payer, [&](auto &a){
            a.username = ahostname;
            a.type = coreahost -> type;
            a.title = coreahost -> title;
            a.purpose = coreahost -> purpose;
            a.manifest = coreahost -> manifest;
            a.comments_is_enabled = coreahost -> comments_is_enabled;
            a.meta = coreahost -> meta;
            a.is_host = coreahost -> is_host;
        });
    
    }


    [[eosio::action]] void unicore::closeahost(eosio::name host){
        require_auth(_self);

        ahosts_index coreahosts(_me, _me.value);
        auto corehost = coreahosts.find(host.value);

        eosio::check(corehost != coreahosts.end(), "Core host is not found");
        
        coreahosts.erase(corehost);

    }

    [[eosio::action]] void unicore::openahost(eosio::name host){
        require_auth(_self);

        ahosts_index coreahosts(_me, _me.value);
        auto corehost = coreahosts.find(host.value);

        eosio::check(corehost == coreahosts.end(), "Core host is already found");
        account_index accounts(_me, _me.value);
        auto acc = accounts.find(host.value);
        
        eosio::check(acc != accounts.end(), "Host is not found");

        coreahosts.emplace(_self, [&](auto &c){
            c.username = acc -> username;
            c.is_host = true;
            c.title = acc -> title;
            c.purpose = acc -> purpose;
            c.comments_is_enabled = false;
            c.meta = acc -> meta;
            c.type = "simple"_n;
        });

    }

    [[eosio::action]] void unicore::rmahost(eosio::name host, eosio::name ahostname){
        eosio::name payer;
        
        conditions_index conditions(_me, host.value);
        eosio::name keyname = name("condjoinhost");
        auto condition = conditions.find(keyname.value);
        

        if (condition != conditions.end() && condition -> value == "architect"_n.value){
            payer = host;
            require_auth(host); 
        } else {
            eosio::check(has_auth(host) || has_auth(ahostname), "missing required authority");
            payer = has_auth(host) ? host : ahostname;
        }


        ahosts_index coreahosts(_me, _me.value);
        auto corehost = coreahosts.find(host.value);

        eosio::check(corehost != coreahosts.end(), "Core host is not found");
        // eosio::check(corehost -> type == "platform"_n, "Host is not a platform");

        ahosts_index ahosts(_me, host.value);
        auto ahost = ahosts.find(ahostname.value);

        eosio::check(ahost != ahosts.end(), "AHOST is not found under platform");
        ahosts.erase(ahost);

    }

    [[eosio::action]] void unicore::setwebsite(eosio::name host, eosio::name ahostname, eosio::string website, eosio::name type, std::string meta){
        require_auth(ahostname);

        ahosts_index coreahosts(_me, _me.value);
        auto corehost = coreahosts.find(ahostname.value);

        eosio::check(corehost != coreahosts.end(), "Core host is not found");
        auto hash = corehost -> hashit(website);
        
        auto hash_index = coreahosts.template get_index<"byuuid"_n>();
        auto exist = hash_index.find(hash);


        if ( exist != hash_index.end() && exist -> username != host && exist->website != "") {
            eosio::check(exist == hash_index.end(), "Current website is already setted to some host");
        }

        if (host.value == ahostname.value) {
            coreahosts.modify(corehost, ahostname, [&](auto &ch){
                ch.website = website;
                ch.hash = hash;
                ch.type = type;
                ch.meta = meta;
            });        
        };

        ahosts_index coreahostsbyusername(_me, _me.value);
        auto coreahostbyusername = coreahostsbyusername.find(ahostname.value);

        ahosts_index ahosts(_me, host.value);
        auto ahost = ahosts.find(ahostname.value);
        
        if (ahost == ahosts.end()){
            
            ahosts.emplace(ahostname, [&](auto &ah) {
                ah.username = coreahostbyusername -> username;
                ah.is_host = coreahostbyusername -> is_host;
                ah.title = coreahostbyusername -> title;
                ah.purpose = coreahostbyusername -> purpose;
                ah.manifest = coreahostbyusername -> manifest;
                ah.comments_is_enabled = coreahostbyusername->comments_is_enabled;
                ah.meta = coreahostbyusername -> meta;
                ah.website = website;
                ah.hash = hash;
                ah.type = type;
            });

        } else {
            
            ahosts.modify(ahost, ahostname, [&](auto &ah){
                ah.website = website;
                ah.hash = hash;
                ah.type = type;
                ah.meta = meta;
            }); 

        }    
    
        


        
    }

    /**
     * @brief      Метод установки уровней вознаграждений
     * @param[in]  op    The operation
     */
    [[eosio::action]] void unicore::setlevels(eosio::name host, std::vector<uint64_t> levels){
        require_auth(host);
        account_index accounts(_me, _me.value);
        
        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Account is not a host");

        //eosio::check for referal levels;
        uint64_t level_count = 0;
        uint64_t percent_count = 0;
        uint64_t prev_level = 100 * ONE_PERCENT;

        for (auto level : levels){
            level_count++;
            eosio::check(level != 0, "Zero-level is prohibited.");
            percent_count += level;
        };

        eosio::check(percent_count == 100 * ONE_PERCENT, "Summ of all levels should be 100%");
        eosio::check(level_count <= _MAX_LEVELS, "Exceed the maximum number of levels");
        
        accounts.modify(acc, host, [&](auto &a){
            a.levels = levels;
        });


    }

    [[eosio::action]] void unicore::setconsensus(eosio::name host, uint64_t consensus_percent, bool voting_only_up){
        account_index accounts(_me, _me.value);
        
        auto acc = accounts.find(host.value);
        eosio::check(acc != accounts.end(), "Account is not a host");

        require_auth(acc -> architect);

        eosio::check(consensus_percent >=0 && consensus_percent <= HUNDR_PERCENT, "Consensus should be more then zero and less then 100 * one_percent");

        accounts.modify(acc, acc -> architect, [&](auto &a){
            a.consensus_percent = consensus_percent;
            a.voting_only_up = voting_only_up;
        });
    }
        
    /**
     * @brief      Метод апгрейда аккаунта до статуса сообщества
     * Принимает ряд параметров, такие как процент консенсуса, реферальный процент, уровни вознаграждений финансовых партнеров, 
     * корпоративный процент, а так же параметры рынка силы. 
     *  
     * @param[in]  op    The operation
     */
    [[eosio::action]] void unicore::upgrade(eosio::name username, eosio::name platform, std::string title, std::string purpose, uint64_t total_shares, eosio::asset quote_amount, eosio::name quote_token_contract, eosio::asset root_token, eosio::name root_token_contract, bool voting_only_up, uint64_t consensus_percent, uint64_t referral_percent, uint64_t dacs_percent, uint64_t cfund_percent, uint64_t hfund_percent, std::vector<uint64_t> levels, uint64_t emission_percent, uint64_t gtop, std::string meta){
        require_auth(username);
        
        // eosio::check(purpose.length() > 0, "Purpose should contain a symbols. Describe the idea shortly.");

        //eosio::check title lenght
        // eosio::check((title.length() < 1024) && (title.length() > 0) , "Title should be more then 10 symbols and less then 1024");
        partners2_index users(_partners, _partners.value);
        
        auto user = users.find(username.value);
        // eosio::check(user != users.end(), "User is not registered");


        eosio::check(title.length() < 100, "Title is a maximum 100 symbols");
        eosio::check(purpose.find("&#") , "Symbols '&#' is prohibited in purpose");

        
        eosio::check(consensus_percent <= 100 * ONE_PERCENT, "consensus_percent should be between 0 and 100 * ONE_PERCENT (1000000)");
        
        eosio::check(referral_percent + dacs_percent + cfund_percent + hfund_percent == 100 * ONE_PERCENT, "All payment percents should equal 100 * ONE_PERCENT (1000000)");
        
        eosio::check(emission_percent <= 1000 * ONE_PERCENT, "emission_percent should be between 0 and 1000 * ONE_PERCENT (10000000)");
        eosio::check(gtop <= 100, "Goal top should be less then 100");

        //eosio::check for referal levels;
        uint64_t level_count = 0;
        uint64_t percent_count = 0;
        uint64_t prev_level = 100 * ONE_PERCENT;

        for (auto level : levels){
            level_count++;
            // eosio::check(level <= prev_level, "Percentage on each referal level should decrease or not change");
            //TODO WHY?
            eosio::check(level != 0, "Zero-level is prohibited.");
            percent_count += level;
        };

        eosio::check(percent_count == 100 * ONE_PERCENT, "Summ of all levels should be 100%");
        eosio::check(level_count <= _MAX_LEVELS, "Exceed the maximum number of levels");
        //END eosio::check

        account_index accounts(_me, _me.value);
        
        auto itr = accounts.find(username.value);
        eosio::check(itr == accounts.end(), "Account is already upgraded to Host");

        auto failure_if_root_not_exist1   = eosio::token::get_supply(root_token_contract, root_token.symbol.code() );

                
        uint64_t to_pay_amount = unicore::getcondition(platform, "hostfee");
        eosio::asset to_pay = asset(to_pay_amount, root_token.symbol);

        //Создаём запись о платформе создания для возврата платежа с оплатой
        account3_index accounts3(_me, _me.value);
        accounts3.emplace(_me, [&](auto &cr){
            cr.username = username;
            cr.platform = platform;
        });

        auto ref = users.find(username.value);
        eosio::name referer = ref->referer;

        std::vector<eosio::name> empty_dacs;
        std::vector<eosio::name> empty_fhosts;

        gpercents_index gpercents(_me, _me.value);
        auto syspercent = gpercents.find("system"_n.value);
        eosio::check(syspercent != gpercents.end(), "Contract is not active");


        accounts.emplace(_me, [&](auto &a){
            a.sale_mode = "direct"_n;
            a.sale_is_enabled = true;

            a.username = username;
            a.architect = username;
            a.hoperator = username;
            a.type = "tin"_n;
            a.chat_mode = "simple"_n;
            a.activated = false;
            a.dac_mode = 0;
            a.total_dacs_weight = 0;
            a.total_shares = total_shares;

            a.quote_amount = asset(0, root_token.symbol);
            a.quote_symbol = root_token.symbol.code().to_string();
            a.quote_token_contract = root_token_contract;
            a.quote_precision = root_token.symbol.precision();
            
            a.asset_on_sale = asset(0, _POWER);
            a.asset_on_sale_precision = _POWER.precision();
            a.asset_on_sale_symbol = _POWER.code().to_string();
            a.sale_shift = 0;
            a.voting_only_up = voting_only_up;
            
            a.root_token = root_token;
            a.symbol = root_token.symbol.code().to_string();
            a.precision = root_token.symbol.precision();
            a.root_token_contract = root_token_contract;
            
            a.meta = meta;
            a.registered_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
            a.payed = to_pay.amount == 0 ? true : false;
            a.to_pay = to_pay;
            a.ahost = username;
            a.fhosts = empty_fhosts;
            a.fhosts_mode = 0;
            a.consensus_percent = consensus_percent;
            a.referral_percent = referral_percent;
            a.dacs_percent = dacs_percent;
            a.cfund_percent = cfund_percent;
            a.hfund_percent = hfund_percent;
            a.sys_percent = syspercent->value;
            a.levels= levels;
            a.title = title;
            a.purpose = purpose;
            a.power_market_id = username;
        });

        ahosts_index coreahosts(_me, _me.value);

        auto coreahost = coreahosts.find(username.value);

        if (coreahost == coreahosts.end()){


            coreahosts.emplace(username, [&](auto &a){
                a.username = username;
                a.title = title;
                a.purpose = purpose;
                a.is_host = true;
                a.manifest = "";
                a.comments_is_enabled = false;
                a.meta = meta;
            });

        }

        emission_index emis(_me, username.value);
        auto emi = emis.find(username.value);
        // eosio::check(emi == emis.end(), "Emission object already created");
        if (emi == emis.end()){
            emis.emplace(username, [&](auto &e){
                e.host = username;
                e.gtop = gtop;
                e.percent = emission_percent;
                e.fund = asset(0, root_token.symbol);
            });
        }

        
    }

    /**
     * @brief      Метод создания дочернего хоста
     * Позволяет сообществу на границе циклов изменять параметры финансового ядра, 
     * распологая их в области памяти аккаунта дочернего хоста. 
     *
     * @param[in]  op    The operation
     */
   [[eosio::action]] void unicore::cchildhost(eosio::name parent_host, eosio::name chost) {
    	account_index hosts(_me, _me.value);
    	auto acc = hosts.find(parent_host.value);
    	eosio::check(acc != hosts.end(), "Parent host is not exist");
    	require_auth(parent_host);
    	eosio::check(acc->activated == true, "Main host should be activated before set a child host");
    	eosio::check( is_account( chost ), "Child account does not exist");
    	
    	std::vector<eosio::name> childs = acc->chosts;

    	//eosio::check for exist in main hosts;
    	auto is_exist = hosts.find(chost.value);
    	eosio::check(is_exist == hosts.end(), "Child host already registered like father host");
		eosio::check(acc->non_active_chost == false, "Founded not activated child host");
    	
    	// //eosio::check for exist not active child
    	if (childs.begin() != childs.end()){
    		eosio::name last_child = childs.back();
		}
    	
    	childs.push_back(chost);

        auto root_symbol = acc->get_root_symbol();
        
    	eosio::asset to_pay = asset(0, root_symbol);
        
    	hosts.modify(acc, parent_host, [&](auto &h){
    		h.chosts = childs;
    		h.payed = true;
    		h.to_pay = to_pay;
    		h.parameters_setted = false;
    		h.non_active_chost = true;
    	});


    }

    /**
     * @brief      Метод оплаты комиссии создания дочернего хоста
     *
     * @param[in]  username  The username
     * @param[in]  amount    The amount
     */
    void unicore::pay_for_upgrade(eosio::name username, eosio::asset amount, eosio::name code){
    	account_index hosts(_me, _me.value);

    	auto host = hosts.find(username.value);
    	eosio::check(host != hosts.end(), "Host is not founded");
        
   		eosio::check(amount.symbol == host->to_pay.symbol, "Wrong symbol for market");
        
        eosio::check(code == host->root_token_contract, "Wrong symbol for market");
        
	   	//eosio::check for enough upgrade amount for quote asset
    	eosio::check(amount == host->to_pay, "Amount is not equal amount for upgrade");
    	eosio::check(host->payed == 0, "Already payed");

    	std::vector<eosio::name> childs = host->chosts;
	   
    	hosts.modify(host, _me, [&](auto &h){
			h.payed = true;
			h.to_pay = asset(0, _SYM);
		});    	

        
    };

    /**
     * @brief      Метод редактирования времени 
     *
     * @param[in]  op    The operation
     */
    [[eosio::action]] void unicore::settiming(eosio::name host, uint64_t pool_timeout, uint64_t priority_seconds) {
        require_auth (host);

        account_index hosts(_me, _me.value);
        auto acc = hosts.find(host.value);
        
        eosio::check((priority_seconds <= 315400000), "Pool Priority Seconds must be greater or equal then 0 sec and less then 315400000 sec");
        eosio::check((pool_timeout >= 60) && (pool_timeout <= 94610000000),"Pool Timeout must be greater or equal then 1 sec and less then 7884000 sec");
        eosio::check(acc -> current_pool_num < 3, "Timing changes only possible on the waiting mode");

        eosio::name main_host = acc->get_ahost();
        spiral_index spiral(_me, main_host.value);
        auto sp = spiral.find(0);

        spiral.modify(sp, host, [&](auto &s){
            s.pool_timeout = pool_timeout;
            s.priority_seconds = priority_seconds;
        });
    }


    [[eosio::action]] void unicore::setflows(eosio::name host, uint64_t ref_percent, uint64_t dacs_percent, uint64_t cfund_percent, uint64_t hfund_percent){
        require_auth (host);        
        
        account_index hosts(_me, _me.value);
        auto acc = hosts.find(host.value);
        
        eosio::check(ref_percent + dacs_percent + cfund_percent + hfund_percent == HUNDR_PERCENT, "All payment percents should equal 100 * ONE_PERCENT (1000000)");
        // eosio::check(acc -> current_pool_num < 3, "Flows changes only possible on the waiting mode");

        hosts.modify(acc, host, [&](auto &h){
            h.referral_percent = ref_percent;
            h.dacs_percent = dacs_percent;
            h.cfund_percent = cfund_percent;
            h.hfund_percent = hfund_percent;
        });

    }

     /**
     * @brief      Метод редактирования информации о хосте
     *
     * @param[in]  op    The operation
     */

    [[eosio::action]] void unicore::edithost(eosio::name architect, eosio::name host, eosio::name platform, eosio::string title, eosio::string purpose, eosio::string manifest, eosio::name power_market_id, eosio::string meta){
        account_index accs(_me, _me.value);
        auto acc = accs.find(host.value);
        
        eosio::check(has_auth(host) || has_auth(acc -> architect), "missing required authority");

        eosio::name payer = has_auth(host) ? host : acc -> architect;


        // eosio::check(acc->architect == architect, "You are not architect of current commquanty");

        eosio::check(acc != accs.end(), "Host is not founded");
        
        // eosio::check(purpose.length() > 0, "Purpose should contain a symbols. Describe the idea shortly.");

        //eosio::check title lenght
        // eosio::check((title.length() < 1024) && (title.length() > 0) , "Title should be more then 10 symbols and less then 1024");

        accs.modify(acc, payer, [&](auto &h){
            h.purpose = purpose;
            h.meta = meta;
            h.title = title;
            // h.power_market_id = power_market_id;
        });

        ahosts_index coreahosts(_me, _me.value);
        auto coreahost = coreahosts.find(host.value);
        eosio::check(coreahost != coreahosts.end(), "AHost is not found");

        coreahosts.modify(coreahost, payer, [&](auto &a){
            a.title = title;
            a.purpose = purpose;
            a.manifest = manifest;
        });

        ahosts_index platformahosts(_me, platform.value);
        auto platformahost = platformahosts.find(host.value);

        if (platformahost == platformahosts.end()){
            platformahosts.emplace(_me, [&](auto &a){
                a.username = host;
                a.type = coreahost -> type;
                a.title = coreahost -> title;
                a.purpose = coreahost -> purpose;
                a.manifest = coreahost -> manifest;
                a.comments_is_enabled = coreahost -> comments_is_enabled;
                a.meta = coreahost -> meta;
                a.is_host = coreahost -> is_host;
            });  
        } else {
            platformahosts.modify(platformahost, payer, [&](auto &a){
                a.title = coreahost -> title;
                a.purpose = coreahost -> purpose;
                a.manifest = coreahost -> manifest;
                a.comments_is_enabled = coreahost -> comments_is_enabled;
                a.meta = coreahost -> meta;
                a.is_host = coreahost -> is_host;
            }); 
        };

    

        ahosts_index ahosts(_me, host.value);
        auto ahost = ahosts.find(host.value);
        if (ahost != ahosts.end()){
            ahosts.modify(ahost, payer, [&](auto &a){
                a.title = title;
                a.purpose = purpose;
                a.manifest = manifest;
            });
        }

    }


// }