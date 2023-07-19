using namespace eosio;


void unicore::add_vbalance(eosio::name username, eosio::name host, eosio::asset quantity){
  //используется при депозите

  vbalance_index vbalances(_me, host.value);
  auto vbalance = vbalances.find(username.value);

  if (vbalance == vbalances.end()) {
    
    vbalances.emplace(_me, [&](auto &vb){
      vb.username = username;
      vb.balance = quantity;
    });

  } else {

    vbalances.modify(vbalance, _me, [&](auto &vb){
      vb.balance += quantity;
    });

  }
      
}



void unicore::sub_vbalance(eosio::name username, eosio::name host, eosio::asset quantity){
  //используется при покупке продукта

  vbalance_index vbalances(_me, host.value);
  auto vbalance = vbalances.find(username.value);
  
  eosio::check(vbalance != vbalances.end(), "Balance is not found");
  eosio::check(vbalance -> balance >= quantity, "Balance is not enought");

  vbalances.modify(vbalance, _me, [&](auto &vb){
    vb.balance -= quantity;
  });
     
}


//вызываем при оплате от контракта
[[eosio::action]] void unicore::payvirtual(eosio::name username, eosio::name from, eosio::name host, eosio::name contract, eosio::asset total, eosio::asset spread_to_refs){
    require_auth(from); 

    // Проверяем, имеет ли контракт разрешение на вызов payvirtual
    eosio::check(check_permission(username, host, from), "No permission to call payvirtual");

    eosio::check(total >= spread_to_refs, "Ref amount cannot be more then total");

    sub_vbalance(username, host, total);

    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);

    eosio::check(acc -> root_token_contract == contract, "Wrong root token contract contract");
    eosio::check(acc != accounts.end(), "Host is not found");

    unicore::spread_to_refs(host, username, spread_to_refs, total, acc -> root_token_contract);

}



bool unicore::get_product_expiration(eosio::name host, eosio::name username, uint64_t product_id) {

    myproducts_index myproducts(_me, host.value); 
    auto myproducts_by_host_index = myproducts.template get_index<"byuserprod"_n>();
    auto myproducts_ids = combine_ids(username.value, product_id);
    auto myproduct = myproducts_by_host_index.find(myproducts_ids);

    eosio::time_point_sec expiration = eosio::time_point_sec(0);

    if (myproduct != myproducts_by_host_index.end()) {
        expiration = myproduct -> expired_at;
    };

    bool is_expired = eosio::current_time_point().sec_since_epoch() >= expiration.sec_since_epoch();

    return is_expired;  
}




