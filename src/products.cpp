using namespace eosio;


[[eosio::action]] void unicore::createprod(name host, eosio::name type, std::string title, std::string description, std::string encrypted_data, std::string public_key, eosio::name token_contract, asset price, uint64_t duration) {

  require_auth(host);

  products_index products(_me, host.value);

  account_index accounts(_me, _me.value);
  auto acc = accounts.find(host.value);

  eosio::check(acc != accounts.end(), "Host is not found");
  eosio::check(acc -> root_token_contract == token_contract, "Wrong root token contract");

  products.emplace(_me, [&](auto &p){
    p.id = unicore::get_global_id("product"_n);
    p.host = host;
    p.title = title;
    p.type = type;
    p.description = description;
    p.public_key = public_key;
    p.encrypted_data = encrypted_data;
    p.price = price;
    p.duration = duration;
    p.token_contract = token_contract;
  });

}



[[eosio::action]] void unicore::editprod(eosio::name host, eosio::name type, uint64_t product_id, std::string title, std::string description, std::string encrypted_data, std::string public_key, asset price, uint64_t duration) {


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
    p.public_key = public_key;
    p.price = price;
    p.duration = duration;
  });
}




[[eosio::action]] void unicore::buyproduct(eosio::name buyer, eosio::name host, uint64_t product_id, uint64_t quantity) {

  require_auth(buyer);
  account_index accounts(_me, _me.value);
  auto acc = accounts.find(host.value);

  eosio::check(acc != accounts.end(), "Host is not found");

  eosio::check(acc -> type == "fractions"_n, "Only if fractions mode enabled product can be boughted");

  products_index products(_me, host.value); 
  auto product = products.find(product_id);
  eosio::check(product != products.end(), "Product is not found");

  eosio::asset amount = product -> price * quantity;
  
  sub_vbalance(buyer, host, amount);

  myproducts_index myproducts(_me, host.value); 
  auto myproducts_by_host_index = myproducts.template get_index<"byhostprod"_n>();
  auto myproducts_ids = combine_ids(host.value, product_id);
  auto myproduct = myproducts_by_host_index.find(myproducts_ids);
  
  if (myproduct == myproducts_by_host_index.end()){
    //emplace
    myproducts.emplace(buyer, [&](auto &p){
      p.id = unicore::get_global_id("myproduct"_n);
      p.host = host;
      p.type = product -> type;
      p.product_id = product_id;
      p.expired_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + quantity * product -> duration);
    });

  } else {
    //modify

    myproducts_by_host_index.modify(myproduct, buyer, [&](auto &p) {
      //если подписка не истекла и это поппокупка
      if (eosio::current_time_point().sec_since_epoch() < myproduct->expired_at.sec_since_epoch()) { 
  
          p.expired_at = eosio::time_point_sec(myproduct->expired_at.sec_since_epoch() + quantity * product -> duration); 
  
      } else { // если подписка истекла начинаем с сейчас
  
          p.expired_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch() + quantity * product -> duration);
  
      };
    });

  }

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


bool unicore::get_product_expiration(eosio::name host, eosio::name username, uint64_t product_id) {

    myproducts_index myproducts(_me, host.value); 
    auto myproducts_by_host_index = myproducts.template get_index<"byhostprod"_n>();
    auto myproducts_ids = combine_ids(host.value, product_id);
    auto myproduct = myproducts_by_host_index.find(myproducts_ids);

    eosio::time_point_sec expiration = eosio::time_point_sec(0);

    if (myproduct != myproducts_by_host_index.end()) {
        expiration = myproduct -> expired_at;
    };

    bool is_expired = eosio::current_time_point().sec_since_epoch() >= expiration.sec_since_epoch();

    return is_expired;  
}




