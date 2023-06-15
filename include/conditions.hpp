/*!
    \brief Структура хранилища универсального набора условий, относящихся к хосту, платформе или протоколу.
*/
    struct  [[eosio::table, eosio::contract("unicore")]] conditions {
        uint64_t key;
        eosio::string key_string;
        uint64_t value;

        uint64_t primary_key() const{return key;}
        EOSLIB_SERIALIZE(conditions, (key)(key_string)(value))
    };

    typedef eosio::multi_index<"conditions"_n, conditions> conditions_index;



/*!
    \brief Структура хранилища универсального набора разрешений, относящихся к аккаунту и хостам на списание виртуальных балансов
    содержится на пользователе
*/
    struct  [[eosio::table, eosio::contract("unicore")]] permissions {
        uint64_t id;
        eosio::name host;
        eosio::name to;
        eosio::string memo;
 

        uint64_t primary_key() const{return id;}
        uint64_t byhost() const {return host.value;}
        uint128_t byhostto() const {return combine_ids(host.value, to.value);} 



        EOSLIB_SERIALIZE(permissions, (id)(host)(to)(memo))
    };

    typedef eosio::multi_index<"permissions"_n, permissions,
        eosio::indexed_by<"byhost"_n, eosio::const_mem_fun<permissions, uint64_t, &permissions::byhost>>,
        eosio::indexed_by< "byhostto"_n, eosio::const_mem_fun<permissions, uint128_t, &permissions::byhostto>>
    
    > permissions_index;
