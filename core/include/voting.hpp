/*!
   \brief Структура голосов за цели хоста Двойной Спирали.
*/

	struct [[eosio::table, eosio::contract("unicore")]] votes{
		uint64_t id;
		uint64_t goal_id;
		eosio::name host;
		int64_t power;

		uint64_t primary_key()const { return id; }
		uint64_t byhost() const {return host.value;}
		uint128_t idwithhost() const { return combine_ids(host.value, goal_id); }

	  EOSLIB_SERIALIZE( votes, (id)(goal_id)(host)(power))
    
    };


    typedef eosio::multi_index< "votes"_n, votes,
    eosio::indexed_by<"host"_n, eosio::const_mem_fun<votes, uint64_t, &votes::byhost>>,
	  eosio::indexed_by<"idwithhost"_n, eosio::const_mem_fun<votes, uint128_t, &votes::idwithhost>>
    > votes_index;





/*!
   \brief Структура голосов за отчёты
*/
	struct [[eosio::table, eosio::contract("unicore")]] rvotes {
		uint64_t id;
		uint64_t task_id;
		uint64_t report_id;
		eosio::name host;
		int64_t power;

		uint64_t primary_key()const { return id; }
		uint64_t byhost() const {return host.value;}
		
		uint128_t idwithhost() const { return combine_ids(host.value, report_id); }
		uint64_t bytaskid() const {return task_id;}

	   EOSLIB_SERIALIZE( rvotes, (id)(task_id)(report_id)(host)(power))
    
    };



   typedef eosio::multi_index< "rvotes"_n, rvotes,
     eosio::indexed_by<"host"_n, eosio::const_mem_fun<rvotes, uint64_t, &rvotes::byhost>>,
	  eosio::indexed_by<"idwithhost"_n, eosio::const_mem_fun<rvotes, uint128_t, &rvotes::idwithhost>>,
	  eosio::indexed_by<"bytaskid"_n, eosio::const_mem_fun<rvotes, uint64_t, &rvotes::bytaskid>>
   > rvotes_index;


