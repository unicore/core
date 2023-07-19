/*!
    \brief Структура хранилища контента и оформления платформ. 
*/
struct  [[eosio::table, eosio::contract("unicore")]] cmscontent {
    eosio::name type;
    eosio::name lang;
    eosio::string content;

    uint64_t primary_key() const {return type.value;}
    uint64_t bylang() const {return lang.value;}

    EOSLIB_SERIALIZE(struct cmscontent, (type)(lang)(content))
  };

  typedef eosio::multi_index<"cmscontent"_n, cmscontent,
    eosio::indexed_by<"bylang"_n, eosio::const_mem_fun<cmscontent, uint64_t, &cmscontent::bylang>>
  
  > cmscontent_index;

/*!
    \brief Структура конфигурации платформы.
*/
  struct  [[eosio::table, eosio::contract("unicore")]] cmsconfig {
    uint64_t id;
    eosio::string config;

    uint64_t primary_key() const {return id;}

    EOSLIB_SERIALIZE(struct cmsconfig, (id)(config))
  };

  typedef eosio::multi_index<"cmsconfig"_n, cmsconfig> cmsconfig_index;