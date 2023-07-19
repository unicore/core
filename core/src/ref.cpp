using namespace eosio;

  
  [[eosio::action]] void unicore::setwithdrwal(eosio::name username, eosio::name host, eosio::string wallet){
    require_auth(username);


    refbalances2_index refbalances2(_me, username.value);
    auto rbalance = refbalances2.find(host.value);
    eosio::check(rbalance != refbalances2.end(), "Ref balance is not founded");
    eosio::check(wallet.size() > 0, "Wallet for withdraw is not setted");

    refbalances2.modify(rbalance, username, [&](auto &rb) {
      rb.wallet = wallet;
    });
  }



  [[eosio::action]] void unicore::complrefwith(eosio::name host, uint64_t id, eosio::string comment){
    require_auth(host);

    usdtwithdraw_index usdt_widthdraws(_me, host.value);
                    
    auto withdraw = usdt_widthdraws.find(id);
    
    eosio::check(withdraw != usdt_widthdraws.end(), "Object is not founded");
    eosio::check(withdraw->host == host, "Hosts is not equal");
    eosio::check(withdraw->status == "process"_n, "Only order on the process status can be completed");
    
    usdt_widthdraws.modify(withdraw, host, [&](auto &w){
      w.status = "complete"_n;
      w.comment = comment;
    });
  }


  [[eosio::action]] void unicore::cancrefwithd(eosio::name host, uint64_t id, eosio::string comment){
    require_auth(host);

    usdtwithdraw_index usdt_widthdraws(_me, host.value);
                    
    auto withdraw = usdt_widthdraws.find(id);
    
    eosio::check(withdraw != usdt_widthdraws.end(), "Withdraw object is not founded");
    eosio::check(withdraw->host == host, "Hosts is not equal");
    eosio::check(withdraw->status == "process"_n, "Only order on the process status can be canceled");

    usdt_widthdraws.modify(withdraw, host, [&](auto &w){
      w.status = "cancel"_n;
      w.comment = comment;
    });

    refbalances2_index refbalances2(_me, withdraw->username.value);
    auto rbalance = refbalances2.find(host.value);

    refbalances2.modify(rbalance, host, [&](auto &rb){
      rb.withdrawed -= withdraw -> to_withdraw;
      rb.available += withdraw -> to_withdraw; 
    });


  };


  /**
   * @brief      Метод вывода остатка партнерского финансового потока
   * withdraw power quant (withpowerun)
  */
  
  [[eosio::action]] void unicore::withrbalance(eosio::name username, eosio::name host){
    require_auth(username);
  
    refbalances2_index refbalances2(_me, username.value);
    auto rbalance = refbalances2.find(host.value);

    eosio::check(rbalance != refbalances2.end(), "Ref balance is not founded");

    uint64_t min_amount = unicore::getcondition(host, "minusdtwithd");
    eosio::check(rbalance->available.amount >= min_amount, "Amount for withdraw should be more or equal minimum withdraw amount");
    eosio::check(rbalance->available.amount > 0, "Amount for withdraw should be more then zero");

    eosio::check(rbalance -> wallet.size() > 0, "Wallet for withdraw is not setted");

    usdtwithdraw_index usdt_widthdraws(_me, host.value);

    usdt_widthdraws.emplace(username, [&](auto &u){
      u.id = usdt_widthdraws.available_primary_key();
      u.created_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
      u.host = host;
      u.username = username;
      u.status = "process"_n;
      u.to_withdraw = rbalance -> available;
      u.wallet = rbalance -> wallet;
    });

    refbalances2.modify(rbalance, username, [&](auto &rb){
      rb.withdrawed += rbalance -> available;
      rb.available -= rbalance -> available;      
    });

  

  }
  /**
   * @brief      Метод вывода партнерского финансового потока
   * withdraw power quant (withpowerun)
  */
  [[eosio::action]] void unicore::withrbenefit(eosio::name username, eosio::name host, uint64_t id){
    require_auth(username);
    account_index account(_me, _me.value);
 
    auto acc = account.find(host.value);
    auto root_symbol = acc->get_root_symbol();
    
    uint64_t refreezesecs = unicore::getcondition(host, "refreezesecs");

    refbalances_index refbalances(_me, username.value);
    
    uint64_t count=0;
    uint64_t on_withdraw = 0;
    
    uint128_t sediment = 0;

    // auto id = ids.begin();

    auto refbalance = refbalances.find(id);
    eosio::check(refbalance != refbalances.end(), "Ref balance is not found");

    if (uint128_t(refbalance->amount.amount * TOTAL_SEGMENTS) < refbalance->segments) {
      sediment += refbalance->segments - uint128_t(refbalance->amount.amount * TOTAL_SEGMENTS);
    }

    on_withdraw += refbalance->amount.amount;
    
    uint64_t level = refbalance -> level;
    
    //TODO get status
    //compare level and current status
    //if not match - skip withdraw without error
    uint64_t current_user_level = unicore::get_status_level("core"_n, username);
    
    //TODO
    // uint64_t is_expired = unicore::get_status_expiration("core"_n, username);
    
    // eosio::check(level <= current_user_level, "You cant withdraw refbalance with your status");
    
    // eosio::check(is_expired == false, "Your status is expired");

    eosio::asset on_widthdraw_asset = asset(on_withdraw, refbalance->amount.symbol);
    
    uint64_t usdtwithdraw = unicore::getcondition(host, "usdtwithdraw");
    bool usdt_withdraw = refbalance->amount.symbol == _USDT && usdtwithdraw > 0;

    if (on_withdraw > 0) {
      
      if (usdt_withdraw) {
        //если есть - зачисление в кошелек таблицы refbalances2
        refbalances2_index refbalances2(_me, username.value);
        auto rbalance2 = refbalances2.find(host.value);
        if (rbalance2 == refbalances2.end()) {

          refbalances2.emplace(username, [&](auto &r2) {
            r2.host = host;
            r2.available = refbalance->amount;
            r2.withdrawed = asset(0, _USDT);
          });

        } else {
          
          refbalances2.modify(rbalance2, username, [&](auto &r2){
            r2.available += refbalance->amount;
          });

        }

      } else if (refreezesecs > 0) {
          eosio::check(root_symbol == on_widthdraw_asset.symbol, "Cant make vesting action with non-root host symbol");

          make_vesting_action(username, host, acc -> root_token_contract, on_widthdraw_asset, refreezesecs, "refwithdraw"_n);  
        
      
      } else {
        eosio::check(root_symbol == on_widthdraw_asset.symbol, "Cant make transfer action with non-root host symbol");
          
        action(
            permission_level{ _me, "active"_n },
            acc->root_token_contract, "transfer"_n,
            std::make_tuple( _me, username, on_widthdraw_asset, std::string("RFLOW-" + (name{username}.to_string() + "-" + (name{host}).to_string()) ))
        ).send();

      }


    } 

    unicore::add_ref_stat(username, acc -> root_token_contract, on_widthdraw_asset);

    refbalances.erase(refbalance);
  
}
