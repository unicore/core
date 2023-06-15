	
/*!
   \brief Структура силы пользователя у хоста Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] power {
		eosio::name host;
    uint64_t power;
		uint64_t staked;
		uint64_t delegated;
    uint64_t with_badges;
		uint64_t primary_key() const {return host.value;}

		EOSLIB_SERIALIZE(struct power, (host)(power)(staked)(delegated)(with_badges))
	};

	typedef eosio::multi_index<"power"_n, power> power_index;

/*!
   \brief Расширение структуры силы пользователя у хоста Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] power2 {
    eosio::name host;
    uint64_t frozen;
    uint64_t primary_key() const {return host.value;}

    EOSLIB_SERIALIZE(struct power2, (host)(frozen))
  };

  typedef eosio::multi_index<"power2"_n, power2> power2_index;



  
/*!
   \brief Обновленная структура силы пользователя у хоста Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] power3 {
    eosio::name username;
    uint64_t power;
    uint64_t staked;
    uint64_t delegated;
    uint64_t with_badges;
    uint64_t frozen;

    uint64_t primary_key() const {return username.value;}

    uint64_t bypower() const {return power;}
    uint64_t bystaked() const {return staked;}
    uint64_t bydelegated() const {return delegated;}
    uint64_t bywbadges() const {return with_badges;}
    uint64_t byfrozen() const {return frozen;}

    EOSLIB_SERIALIZE(struct power3, (username)(power)(staked)(delegated)(with_badges)(frozen))
  };

  typedef eosio::multi_index<"power3"_n, power3,
    eosio::indexed_by<"bypower"_n, eosio::const_mem_fun<power3, uint64_t, &power3::bypower>>,
    eosio::indexed_by<"bystaked"_n, eosio::const_mem_fun<power3, uint64_t, &power3::bystaked>>,
    eosio::indexed_by<"bydelegated"_n, eosio::const_mem_fun<power3, uint64_t, &power3::bydelegated>>,
    eosio::indexed_by<"bywbadges"_n, eosio::const_mem_fun<power3, uint64_t, &power3::bywbadges>>,
    eosio::indexed_by<"byfrozen"_n, eosio::const_mem_fun<power3, uint64_t, &power3::byfrozen>>
  > power3_index;


  
/*!
   \brief Обновленная структура силы пользователя у хоста Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] loyality {
    eosio::name username;
    uint64_t power;
    
    uint64_t primary_key() const {return username.value;}

    uint64_t bypower() const {return power;}
    
    EOSLIB_SERIALIZE(struct loyality, (username)(power))
  };

  typedef eosio::multi_index<"loyality"_n, loyality,
    eosio::indexed_by<"bypower"_n, eosio::const_mem_fun<loyality, uint64_t, &loyality::bypower>>
  > loyality_index;



/*!
   \brief Обновленная структура силы пользователя у хоста Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] quants {
    eosio::name username;
    uint64_t power;
    
    uint64_t primary_key() const {return username.value;}

    uint64_t bypower() const {return power;}


    EOSLIB_SERIALIZE(struct quants, (username)(power))
  };

  typedef eosio::multi_index<"quants"_n, quants,
    eosio::indexed_by<"byquants"_n, eosio::const_mem_fun<quants, uint64_t, &quants::bypower>>
  > quants_index;


/*!
   \brief Обновленная структура силы пользователя у хоста Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] goldens {
    eosio::name host;
    uint64_t total_golden;
    
    uint64_t primary_key() const {return host.value;}

    
    EOSLIB_SERIALIZE(struct goldens, (host)(total_golden))
  };

  typedef eosio::multi_index<"goldens"_n, goldens> goldens_index;



/*!
   \brief Структура статистики распределения безусловного потока жетонов хоста Двойной Спирали. 
*/

  struct [[eosio::table, eosio::contract("unicore")]] powerstat {
    uint64_t id;
    eosio::time_point_sec window_open_at;
    eosio::time_point_sec window_closed_at;
    uint64_t liquid_quants;
    uint64_t liquid_loyality;

    eosio::asset total_available;
    eosio::asset total_remain;
    eosio::asset total_distributed;

    eosio::asset total_partners_available;
    eosio::asset total_partners_distributed; 
       
    uint64_t primary_key() const {return id;}
    uint64_t byopen() const {return window_open_at.sec_since_epoch();}
        

    EOSLIB_SERIALIZE(struct powerstat, (id)(window_open_at)(window_closed_at)(liquid_quants)(liquid_loyality)(total_available)(total_remain)(total_distributed)(total_partners_available)(total_partners_distributed))
  };

  typedef eosio::multi_index<"powerstat"_n, powerstat,
    eosio::indexed_by<"byopen"_n, eosio::const_mem_fun<powerstat, uint64_t, &powerstat::byopen>>
  > powerstat_index;



/*!
   \brief Структура истории преобретения силы пользователем у хоста Двойной Спирали.
*/
  struct [[eosio::table, eosio::contract("unicore")]] powerlog {
    uint64_t id; //auto by cycle
    eosio::name host;
    eosio::name username;
    uint64_t window_id;
    uint64_t loyality_power;
    uint64_t quants_power;
    bool updated = false;
    eosio::asset available_by_loyality;
    eosio::asset available_by_quants;
    eosio::asset distributed_to_partners;

    uint64_t primary_key() const {return id;}
    
    uint128_t userwindowid() const { return combine_ids(username.value, window_id); }
    
    EOSLIB_SERIALIZE(struct powerlog, (id)(host)(username)(window_id)(loyality_power)(quants_power)(updated)(available_by_loyality)(available_by_quants)(distributed_to_partners))
  };

  typedef eosio::multi_index<"powerlog"_n, powerlog,
    eosio::indexed_by<"userwindowid"_n, eosio::const_mem_fun<powerlog, uint128_t, &powerlog::userwindowid>>
  > powerlog_index;


/*!
   \brief Структура истории получения безусловного потока жетонов пользователем у хоста Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] dlog {
    uint64_t id;
    eosio::name host;
    uint64_t pool_id;
    uint128_t segments;
    eosio::asset amount;
    

    uint64_t primary_key() const {return id;}
    
    EOSLIB_SERIALIZE(struct dlog, (id)(host)(pool_id)(segments)(amount))
  };

  typedef eosio::multi_index<"dlog"_n, dlog> dlog_index;

/*!
   \brief Структура учёта делегированной силы от пользователя к пользователю.
*/

	struct [[eosio::table, eosio::contract("unicore")]] delegations {
		eosio::name reciever;
		uint64_t shares;

		uint64_t primary_key() const {return reciever.value;}

		EOSLIB_SERIALIZE(delegations, (reciever)(shares))
	};

	typedef eosio::multi_index<"delegations"_n, delegations> delegation_index;


/*!
   \brief Структура учёта вестинг-балансов пользователей при возврате силы хосту Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] vesting {
  	uint64_t id;
    eosio::name host;
  	eosio::name owner;
    eosio::name contract;
    eosio::name type;
  	eosio::time_point_sec startat;
  	uint64_t duration;
  	eosio::asset amount;
  	eosio::asset available;
  	eosio::asset withdrawed;

  	uint64_t primary_key() const {return id;}

  	EOSLIB_SERIALIZE(vesting, (id)(host)(owner)(contract)(type)(startat)(duration)(amount)(available)(withdrawed))
  };

  typedef eosio::multi_index<"vesting"_n, vesting> vesting_index;




/*!
   \brief Структура учёта партнёров и их статусов у хоста Двойной Спирали.
*/
    struct  [[eosio::table, eosio::contract("unicore")]] corepartners {
        eosio::name username;
        eosio::name status;
        eosio::asset total_good;
        eosio::asset sediment;
        eosio::time_point_sec join_at;
        eosio::time_point_sec expiration;

        uint64_t primary_key() const {return username.value;}
        uint64_t bygood() const {return total_good.amount;}
        uint64_t byexpiration() const {return expiration.sec_since_epoch();}

        EOSLIB_SERIALIZE(corepartners, (username)(status)(total_good)(sediment)(join_at)(expiration))
    };

    typedef eosio::multi_index<"corepartners"_n, corepartners,
      eosio::indexed_by<"bygood"_n, eosio::const_mem_fun<corepartners, uint64_t, &corepartners::bygood>>,
      eosio::indexed_by<"byexpiration"_n, eosio::const_mem_fun<corepartners, uint64_t, &corepartners::byexpiration>>
    > corepartners_index;


