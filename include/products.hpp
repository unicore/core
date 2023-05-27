/*!
   \brief Структура партнёров и их партнёров.
*/

    struct [[eosio::table, eosio::contract("unicore")]] products {
        uint64_t id;
        eosio::name host;
        eosio::name type;
        std::string title;
        uint64_t duration;
        std::string description;
        std::string encrypted_data;
        std::string public_key;
        eosio::name token_contract;
        eosio::asset price;
        
        std::string meta;
        
        uint64_t primary_key() const{return id;}
        uint64_t bycreator() const{return host.value;}
        uint64_t bytype() const{return type.value;}
        
        
        EOSLIB_SERIALIZE(products, (id)(host)(type)(title)(duration)(description)(encrypted_data)(public_key)(token_contract)(price)(meta))


    };
      
    typedef eosio::multi_index<"products"_n, products,
      eosio::indexed_by<"bycreator"_n, eosio::const_mem_fun<products, uint64_t, &products::bycreator>>,
      eosio::indexed_by<"bytype"_n, eosio::const_mem_fun<products, uint64_t, &products::bytype>>
    > products_index;






/*!
   \brief Структура полученных реферальных балансов от партнёров на партнёра.
*/
    struct [[eosio::table, eosio::contract("unicore")]] myproducts {
        uint64_t id;
        eosio::name host;
        eosio::name type;
        uint64_t product_id;
        
        std::string encrypted_data;
        std::string public_key;
        eosio::time_point_sec expired_at;
        std::string meta;
        uint64_t primary_key() const {return id;}
        uint128_t byhostprod() const {return combine_ids(host.value, product_id);}


        EOSLIB_SERIALIZE(myproducts, (id)(host)(type)(product_id)(encrypted_data)(public_key)(expired_at)(meta))
    };


    typedef eosio::multi_index<"myproducts"_n, myproducts,
      eosio::indexed_by< "byhostprod"_n, eosio::const_mem_fun<myproducts, uint128_t, &myproducts::byhostprod>>
    > myproducts_index;




/*!
   \brief Структура виртуального баланса
*/
    struct [[eosio::table, eosio::contract("unicore")]] vbalances {
        eosio::name username;
        eosio::asset balance;

        uint64_t primary_key() const {return username.value;}
        

        EOSLIB_SERIALIZE(vbalances, (username)(balance))
    };


    typedef eosio::multi_index<"vbalances"_n, vbalances> vbalance_index;

