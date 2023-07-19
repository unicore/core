
// using namespace eosio;

// /**
//  * @brief      Модуль IPFS
//  * Позволяет хранить ключи доступа к интерпланетарной файловой системе, 
//  * создавать публичные пути орбитальной базы данных, 
//  * а так же покупать и продавать зашифрованные данные. 
//  */


//   [[eosio::action]] void unicore::setstorage(eosio::name username, uint64_t id, eosio::string name, eosio::string address){
//     require_auth(username);

//     storage_index storages(_me, username.value);
//     auto storage = storages.find(id);
    
//     if (storage == storages.end()){
      
//       storages.emplace(username, [&](auto &s){
//         s.id = id;
//         s.address = address;
//         s.name = name;
//       });

//     } else {
//       storages.modify(storage, username, [&](auto &s){
//         s.address = address;
//         s.name = name;
//       });

//     };
//   }

//   [[eosio::action]] void unicore::removeroute(eosio::name username, uint64_t id){
//     require_auth(username);
//     storage_index storages(_me, username.value);
//     auto storage = storages.find(id);
//     storages.erase(storage);
//   }

//   [[eosio::action]] void unicore::setipfskey (eosio::name username, eosio::string pkey, eosio::string meta){
//     require_auth(username);

//     ipfskeys_index ipfskeys(_me, username.value);
    
//     ipfskeys.emplace(username, [&](auto &i){
//       i.id = ipfskeys.available_primary_key();
//       i.pkey = pkey;
//       i.meta = meta;
//     });
//   }



//   [[eosio::action]] void unicore::selldata(eosio::name username, uint64_t id, eosio::string data, eosio::name root_token_contract, eosio::asset amount){
//     require_auth(username);

//     orbdata_index orbdts(_me, username.value);
//     auto data_exist = orbdts.find(id);
//     eosio::check(data_exist == orbdts.end(), "Cannot use same ID");
//     auto failure_if_root_not_exist   = eosio::token::get_supply(root_token_contract, amount.symbol.code() );
//     orbdts.emplace(username, [&](auto &ds){
//       ds.id = id;
//       ds.data = data;
//       ds.root_token_contract = root_token_contract;
//       ds.amount = amount;
//     });
//   }

//   void unicore::buydata_action(eosio::name owner, uint64_t data_id, eosio::name buyer, eosio::asset quantity, eosio::name code){
    
//     orbdata_index orbdts(_me, owner.value);
//     auto data = orbdts.find(data_id);
//     eosio::check(data != orbdts.end(), "Data ID is not found. Rejected.");

//     eosio::check(data -> root_token_contract == code, "Wrong token contract for this data quant. Rejected");
//     eosio::check(data -> amount == quantity, "Recieved amount is not equal with data cost. Rejected");

//     dataorders_index oorders(_me, owner.value);

//     //TODO eosio::check for order exist by data_id and buyer;


//     auto buyers_with_data_id = oorders.template get_index<"buyerandid"_n>();

//     auto buyers_with_data_id_ids = combine_ids(buyer.value, data->id);
    
//     auto exist = buyers_with_data_id.find(buyers_with_data_id_ids);

//     eosio::check(exist == buyers_with_data_id.end(), "Order from buyer already exist for this data-quant");

//     auto id = oorders.available_primary_key();
    
//     oorders.emplace(buyer, [&](auto &oo){
//       oo.id = id;
//       oo.orbdata_id = data_id;
//       oo.opened_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
//       oo.owner = owner;
//       oo.buyer = buyer;
//       oo.curator = _curator;
//       oo.locked_amount = quantity;
//       oo.expired_at = eosio::time_point_sec(-1);
//     });

//       mydataordrs_index mydataordrs (_me, buyer.value);

//       auto seller_with_order_id_idx = combine_ids(owner.value, id);

//       mydataordrs.emplace(_me, [&](auto &ml){
//         ml.owner = owner;
//         ml.order_id = id;
//       });
 
//   }


//   [[eosio::action]] void unicore::dataapprove(eosio::name username, eosio::name owner, eosio::name who, uint64_t order_id, eosio::string message){
//     require_auth(username);

//     dataorders_index oorders(_me, owner.value);
//     auto order = oorders.find(order_id);
//     eosio::check(order != oorders.end(), "Order is not found");
    
//     orbdata_index orbdts(_me, owner.value);
//     auto data = orbdts.find(order -> orbdata_id);
//     eosio::check(data != orbdts.end(), "Data is not found");

//     //If this is data-owner, message -> is key.

//     if (username == order -> owner){
      
//       oorders.modify(order, username, [&](auto &oo){
//         oo.approved = true;
//         oo.key = message;
//         oo.expired_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch()+ _DATA_ORDER_EXPIRATION);
//       });

//     } else if (username == order -> buyer){

//       eosio::check(order -> approved == true, "Cannot finish order before approve from owner");

//       action(
//             permission_level{ _me, "active"_n },
//             data->root_token_contract, "transfer"_n,
//             std::make_tuple( _me, order->owner, order -> locked_amount, std::string("Order is closed positive.")) 
//       ).send();

      
//       orbdts.modify(data, username, [&](auto &oo){
//         oo.sales_count = data -> sales_count + 1;
//       });

//       oorders.erase(order);
      
//       mydataordrs_index mydataordrs (_me, (order->buyer).value);

//       auto seller_with_order_id_idx = combine_ids((order->owner).value, order-> id);

//       auto sellers_with_my_order = mydataordrs.find(seller_with_order_id_idx);

//       mydataordrs.erase(sellers_with_my_order);
      
//       userdatacounts_index usercounts(_me, _me.value);

//       auto buyercounts = usercounts.find(username.value);
//       auto ownercounts = usercounts.find((order-> owner).value);

//       if (buyercounts == usercounts.end()){
        
//         usercounts.emplace(_me, [&](auto &uc){
//           uc.username = username;
//           uc.total_buys = 1;
//         });

//       } else {

//         usercounts.modify(buyercounts, _me, [&](auto &uc){
//           uc.total_buys = buyercounts -> total_buys + 1;
//         });

//       }

//       if (ownercounts == usercounts.end()){
        
//         usercounts.emplace(_me, [&](auto &uc){
//           uc.username = order-> owner;
//           uc.total_sales = 1;
//         });

//       } else {
        
//         usercounts.modify(ownercounts, _me, [&](auto &uc){
//           uc.total_sales = ownercounts -> total_sales + 1;
//         });

//       }

//       //TODO add comment to data
//       //TODO add likes depends on data
//     } else if (username == order -> curator){
//       //TODO curator cannot do nothing until disput open 

//       //TODO transfer tokens to WHO
//       //TODO modificate counts
//       //
//     }

//   };
