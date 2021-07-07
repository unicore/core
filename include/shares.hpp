	
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
    uint64_t emitted;
    uint64_t primary_key() const {return host.value;}

    EOSLIB_SERIALIZE(struct power2, (host)(emitted))
  };

  typedef eosio::multi_index<"power2"_n, power2> power2_index;



/*!
   \brief Структура статистики распределения безусловного потока жетонов хоста Двойной Спирали. 
*/

  struct [[eosio::table, eosio::contract("unicore")]] pstats {
    eosio::name host;
    eosio::asset total_available_in_asset;
    uint64_t pflow_last_withdrawed_pool_id;
    uint128_t pflow_available_segments;
    eosio::asset pflow_available_in_asset;
    eosio::asset pflow_withdrawed;

    eosio::asset ref_available_in_asset;
    uint64_t ref_available_segments = 0;
    eosio::asset ref_withdrawed;
    uint64_t primary_key() const {return host.value;}

    EOSLIB_SERIALIZE(struct pstats, (host)(total_available_in_asset)(pflow_last_withdrawed_pool_id)(pflow_available_segments)(pflow_available_in_asset)(pflow_withdrawed)(ref_available_in_asset)(ref_available_segments)(ref_withdrawed))
  };

  typedef eosio::multi_index<"pstats"_n, pstats> pstats_index;


/*!
   \brief Структура истории преобретения силы пользователем у хоста Двойной Спирали.
*/

  struct [[eosio::table, eosio::contract("unicore")]] plog {
    uint64_t id;
    eosio::name host;
    uint64_t pool_id;
    uint64_t cycle_num;
    uint64_t pool_num;
    uint64_t power;
    

    uint64_t primary_key() const {return id;}
    
    uint128_t hostpoolid() const { return combine_ids(host.value, pool_id); }
    
    EOSLIB_SERIALIZE(struct plog, (id)(host)(pool_id)(cycle_num)(pool_num)(power))
  };

  typedef eosio::multi_index<"plog"_n, plog,
    eosio::indexed_by<"hostpoolid"_n, eosio::const_mem_fun<plog, uint128_t, &plog::hostpoolid>>
  > plog_index;


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
  	eosio::time_point_sec startat;
  	uint64_t duration;
  	eosio::asset amount;
  	eosio::asset available;
  	eosio::asset withdrawed;

  	uint64_t primary_key() const {return id;}

  	EOSLIB_SERIALIZE(vesting, (id)(host)(owner)(startat)(duration)(amount)(available)(withdrawed))
  };

  typedef eosio::multi_index<"vesting"_n, vesting> vesting_index;


/*!
   \brief Структура учёта партнёров и их статусов у хоста Двойной Спирали.
*/
    struct  [[eosio::table, eosio::contract("unicore")]] cpartners2 {
        eosio::name partner;
        eosio::name status;
        eosio::time_point_sec join_at;
        eosio::time_point_sec expiration;

        uint64_t primary_key() const {return partner.value;}
        EOSLIB_SERIALIZE(cpartners2, (partner)(status)(join_at)(expiration))
    };

    typedef eosio::multi_index<"cpartners2"_n, cpartners2> cpartners2_index;


