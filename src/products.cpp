using namespace eosio;



[[eosio::action]] void unicore::addflow(name host, uint64_t product_id, eosio::time_point_sec start_at, eosio::time_point_sec closed_at, std::string encrypted_data, std::string meta) {

  require_auth(host);

  products_index products(_me, host.value);

  account_index accounts(_me, _me.value);
  auto acc = accounts.find(host.value);

  eosio::check(acc != accounts.end(), "Host is not found");
  eosio::check(acc -> type == "tin"_n, "Only in simple mode of double helix products can be used");

  auto product = products.find(product_id);
  eosio::check(product != products.end(), "Product is not found");

  eosio::check(product -> type == "info"_n, "Only info products can be selled by flows");

  flows_index flows(_me, host.value);
  
  flows.emplace(_me, [&](auto &f){
    f.id = unicore::get_global_id("flow"_n);
    f.host = host;
    f.product_id = product_id;
    f.start_at = start_at;
    f.closed_at = closed_at;
    f.encrypted_data = encrypted_data;
    f.meta = meta;
  });

}



[[eosio::action]] void unicore::createprod(name host, eosio::name type, uint64_t referral_percent, std::string title, std::string description, std::string encrypted_data, eosio::name token_contract, asset price, uint64_t duration) {

  require_auth(host);

  products_index products(_me, host.value);

  account_index accounts(_me, _me.value);
  auto acc = accounts.find(host.value);

  eosio::check(acc != accounts.end(), "Host is not found");
  eosio::check(acc -> root_token_contract == token_contract, "Wrong root token contract");
  eosio::check(acc -> type == "tin"_n, "Only in simple mode of double helix products can be used");
  spiral_index spiral(_me, host.value);
  auto sp = spiral.find(0);
  eosio::check(sp -> loss_percent < HUNDR_PERCENT, "Comission for product cannot be 100%");
  
  eosio::check(referral_percent <= HUNDR_PERCENT, "Ref percent cannot be more then 100%");

  auto root_symbol = acc->get_root_symbol();

  products.emplace(_me, [&](auto &p){
    p.id = unicore::get_global_id("product"_n);
    p.host = host;
    p.title = title;
    p.type = type;
    p.description = description;
    p.referral_percent = referral_percent;
    p.price = price;
    p.referral_amount = price * referral_percent / HUNDR_PERCENT;
    p.duration = duration;
    p.token_contract = token_contract;
    p.total = price  + price * referral_percent / HUNDR_PERCENT;
    p.total_solded_amount = asset(0, root_symbol);
    p.total_solded_count = 0;
  });

}



[[eosio::action]] void unicore::editprod(eosio::name host, eosio::name type, uint64_t referral_percent, uint64_t product_id, std::string title, std::string description, std::string encrypted_data, asset price, uint64_t duration) {


  require_auth(host);

  products_index products(_me, host.value);

  auto product = products.find(product_id);
  eosio::check(product != products.end(), "Product is not found");

  account_index accounts(_me, _me.value);
  auto acc = accounts.find(host.value);

  eosio::check(acc != accounts.end(), "Host is not found");
  
  products.modify(product, _me, [&](auto &p){
    p.title = title;
    p.description = description;
    p.encrypted_data = encrypted_data;
    p.price = price;
    p.duration = duration;
    p.referral_percent = referral_percent;
    p.price = price;
    p.referral_amount = price * referral_percent / HUNDR_PERCENT;
    p.total = price  + price * referral_percent / HUNDR_PERCENT;
  });
}




[[eosio::action]] void unicore::buysyssubscr(eosio::name buyer, eosio::name host, uint64_t product_id, uint64_t quantity) {

  require_auth(buyer);
  account_index accounts(_me, _me.value);
  auto acc = accounts.find(host.value);

  eosio::check(acc != accounts.end(), "Host is not found");

  eosio::check(acc -> type == "tin"_n, "Only if simple mode enabled product can be boughted");

  products_index products(_me, host.value); 
  auto product = products.find(product_id);
  eosio::check(product != products.end(), "Product is not found");

  eosio::asset amount = product -> price * quantity;
  eosio::asset ref_amount = product -> referral_amount * quantity;
  eosio::asset total = product -> total * quantity;
  
  eosio::check(amount + ref_amount == total, "Prices and amounts is not equal");
  eosio::check(product -> type == ""_n, "Only system subscription can be boughted by this method");

  sub_vbalance(buyer, host, total);

  unicore::spread_to_refs(host, buyer, ref_amount, total, acc -> root_token_contract);

  myproducts_index myproducts(_me, host.value); 
  auto myproducts_by_host_index = myproducts.template get_index<"byuserprod"_n>();
  auto myproducts_ids = combine_ids(buyer.value, product_id);
  auto myproduct = myproducts_by_host_index.find(myproducts_ids);
  
  if (myproduct == myproducts_by_host_index.end()){
    //emplace
    myproducts.emplace(buyer, [&](auto &p){
      p.id = unicore::get_global_id("myproduct"_n);
      p.host = host;
      p.username = buyer;
      p.type = product -> type;
      p.product_id = product_id;
      p.expired_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + quantity * product -> duration);
      p.total = total;
    });

  } else {
    //modify

    myproducts_by_host_index.modify(myproduct, buyer, [&](auto &p) {
      //если подписка не истекла и это поппокупка
      p.total += total;

      if (eosio::current_time_point().sec_since_epoch() < myproduct->expired_at.sec_since_epoch()) { 
  
          p.expired_at = eosio::time_point_sec(myproduct->expired_at.sec_since_epoch() + quantity * product -> duration); 
    
      } else { // если подписка истекла начинаем с сейчас
  
          p.expired_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + quantity * product -> duration);
  
      };
    });

  }

}




[[eosio::action]] void unicore::buysecret(eosio::name buyer, eosio::name host, uint64_t flow_id) {

  require_auth(buyer);
  account_index accounts(_me, _me.value);
  auto acc = accounts.find(host.value);

  eosio::check(acc != accounts.end(), "Host is not found");

  eosio::check(acc -> type == "tin"_n, "Only if simple mode enabled product can be boughted");

  flows_index flows(_me, host.value);
  auto flow = flows.find(flow_id);
  

  products_index products(_me, host.value); 
  auto product = products.find(flow -> product_id);
  eosio::check(product != products.end(), "Product is not found");

  eosio::asset amount = product -> price;
  eosio::asset ref_amount = product -> referral_amount;
  eosio::asset total = product -> total;
  
  eosio::check(amount + ref_amount == total, "Prices and amounts is not equal");

  eosio::check(product -> type == "info"_n, "Only info products can be boughted by this method");

  flows.modify(flow, buyer, [&](auto &f){
    f.total += total;
    f.participants += 1;
  });

  products.modify(product, buyer, [&](auto &p){
    p.total += total;
  });

  sub_vbalance(buyer, host, total);

  unicore::spread_to_refs(host, buyer, ref_amount, total, acc -> root_token_contract);

  myproducts_index myproducts(_me, host.value); 
  auto myproducts_by_host_index = myproducts.template get_index<"byuserprod"_n>();

  auto myproducts_ids = combine_ids(buyer.value, flow -> product_id);
  auto myproduct = myproducts_by_host_index.find(myproducts_ids);
  
  uint64_t user_product_id = unicore::get_global_id("myproduct"_n);

  myproducts.emplace(buyer, [&](auto &p){
      p.id = user_product_id;
      p.host = host;
      p.username = buyer;
      p.type = product -> type;
      p.product_id = product -> id;
      p.flow_id = flow_id;
      p.total = total;
      // p.expired_at = eosio::time_point_sec(flow -> closed_at.sec_since_epoch() + product -> duration);
    });


}



[[eosio::action]] void unicore::deriveprod(eosio::name host, uint64_t user_product_id, std::string encrypted_data) {
  require_auth(host); 

  myproducts_index myproducts(_me, host.value); 
  
  auto myproduct = myproducts.find(user_product_id);

  myproducts.modify(myproduct, host, [&](auto &p){
    p.encrypted_data = encrypted_data;
  });
  
}


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




