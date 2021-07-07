/*!
   \brief Структура задач хоста Двойной Спирали.
*/
	struct [[eosio::table, eosio::contract("unicore")]] tasks {
		uint64_t task_id;
		uint64_t goal_id;
        eosio::name host;
        eosio::name creator;
        eosio::name benefactor;
        eosio::name suggester;
        std::string permlink;
        eosio::name type;
        eosio::name status;
		uint64_t priority;
		bool is_regular;
        std::vector<uint64_t> calendar;
		bool is_public = true;
        eosio::name doer;
		eosio::name role;
        uint64_t level;
        eosio::string title;
		eosio::string data;
		eosio::asset requested;
		eosio::asset funded;
		eosio::asset remain;
		eosio::asset for_each;
		eosio::name curator;
		uint64_t gifted_badges = 0;
        uint64_t gifted_power = 0;
        uint64_t reports_count = 0;
        bool with_badge;
		uint64_t badge_id;
        bool validated = false;
		bool completed = false;
		bool active = true;
        eosio::time_point_sec created_at;
        eosio::time_point_sec start_at;
		eosio::time_point_sec expired_at;
        bool is_batch = false;
        std::vector<uint64_t> batch;
        uint64_t parent_batch_id;
        uint64_t duration;
        bool is_encrypted = false;
        std::string public_key;
        int64_t total_votes;
        std::vector<eosio::name> voters;

        std::string meta;


		uint64_t primary_key()const { return task_id; }
        
        uint64_t bycreator() const {return creator.value;}
        uint64_t bycurator() const {return curator.value;}
        uint64_t bybenefactor() const {return benefactor.value;}
        uint64_t bygoal() const {return goal_id; }
        uint128_t goalandtask() const { return combine_ids(goal_id, task_id); }
        uint64_t byhost() const {return host.value; }
        uint64_t bytype() const {return type.value; }
        uint64_t bystatus() const {return status.value; }
        uint64_t bypriority() const {return priority; }
        uint64_t byhasbadge() const {return with_badge; }
        uint64_t bybadge() const {return badge_id; }
        uint128_t crewithtask() const { return combine_ids(creator.value, task_id); }
        uint128_t crewithgoal() const { return combine_ids(creator.value, goal_id); }
        
        uint64_t byvotes() const { 
            return pow(2, 63) + total_votes;
        }

	    EOSLIB_SERIALIZE( tasks, (task_id)(goal_id)(host)(creator)(benefactor)(suggester)(permlink)(type)(status)(priority)(is_regular)(calendar)(is_public)(doer)(role)(level)(title)(data)(requested)(funded)(remain)(for_each)(curator)(gifted_badges)(gifted_power)(reports_count)(with_badge)(badge_id)(validated)(completed)(active)(created_at)(start_at)(expired_at)(is_batch)(batch)(parent_batch_id)(duration)(is_encrypted)(public_key)(total_votes)(voters)(meta))
    };

    typedef eosio::multi_index< "tasks"_n, tasks,
        eosio::indexed_by<"byhost"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::byhost>>,
        eosio::indexed_by<"bygoal"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bygoal>>,
        eosio::indexed_by<"goalandtask"_n, eosio::const_mem_fun<tasks, uint128_t, &tasks::goalandtask>>,
        eosio::indexed_by<"bytype"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bytype>>,
        eosio::indexed_by<"bypriority"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bypriority>>,
        eosio::indexed_by<"byhasbadge"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::byhasbadge>>,
        eosio::indexed_by<"bybadge"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bybadge>>,
        eosio::indexed_by<"crewithtask"_n, eosio::const_mem_fun<tasks, uint128_t, &tasks::crewithtask>>,
        eosio::indexed_by<"crewithgoal"_n, eosio::const_mem_fun<tasks, uint128_t, &tasks::crewithgoal>>,
        eosio::indexed_by<"bycreator"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bycreator>>,
        eosio::indexed_by<"bycurator"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bycurator>>,
        eosio::indexed_by<"byvotes"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::byvotes>>,
        eosio::indexed_by<"bystatus"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bystatus>>
    > tasks_index;



/*!
   \brief Структура отчетов по задачам хоста Двойной Спирали.
*/
    struct [[eosio::table, eosio::contract("unicore")]] reports {
    	uint64_t report_id;
    	uint64_t task_id; 
    	uint64_t goal_id;
        uint64_t type;
        uint64_t count;
    	eosio::name username;
    	eosio::name curator;
    	eosio::string data;
    	eosio::asset requested;
        eosio::asset balance;
        eosio::asset withdrawed;
    	bool need_check = true;
    	bool approved = false;
    	eosio::string comment;
        eosio::time_point_sec created_at;
        eosio::time_point_sec expired_at;

    	
        uint64_t primary_key() const {return report_id;}
		uint64_t byusername() const {return username.value;}
        uint128_t userwithgoal() const { return combine_ids(username.value, goal_id); }
        uint128_t userwithtask() const { return combine_ids(username.value, task_id); }
        

    	EOSLIB_SERIALIZE(reports, (report_id)(task_id)(goal_id)(type)(count)(username)(curator)(data)(requested)(balance)(withdrawed)(need_check)(approved)(comment)(created_at)(expired_at))
    };

    typedef eosio::multi_index< "reports"_n, reports,
        eosio::indexed_by<"userwithtask"_n, eosio::const_mem_fun<reports, uint128_t, &reports::userwithtask>>,
        eosio::indexed_by<"userwithgoal"_n, eosio::const_mem_fun<reports, uint128_t, &reports::userwithgoal>>,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<reports, uint64_t, &reports::byusername>>        
        
    > reports_index;



/*!
   \brief Структура деятелей по задаче.
*/
    struct [[eosio::table, eosio::contract("unicore")]] doers {
        uint64_t id;
        uint64_t task_id; 
        eosio::name doer;
        bool is_host;
        uint64_t doer_goal_id;
        uint64_t doer_badge_id;
        std::string comment;
        
        uint64_t primary_key() const {return id;}
        uint64_t byusername() const {return doer.value;}
        uint128_t doerwithtask() const { return combine_ids(doer.value, task_id); }

        EOSLIB_SERIALIZE(doers, (id)(task_id)(doer)(is_host)(doer_goal_id)(doer_badge_id)(comment))
    };

    typedef eosio::multi_index< "doers"_n, doers,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<doers, uint64_t, &doers::byusername>>,        
        eosio::indexed_by<"doerwithtask"_n, eosio::const_mem_fun<doers, uint128_t, &doers::doerwithtask>>
        
    > doers_index;


/*!
   \brief Структура задач, где аккаунт деятель.
*/
    struct [[eosio::table, eosio::contract("unicore")]] iamdoer {
        uint64_t id;
        eosio::name host;
        uint64_t task_id; 
        
        uint64_t primary_key() const {return id;}
        uint64_t byhost() const {return host.value;}
        uint128_t byhosttask() const { return combine_ids(host.value, task_id); }

        EOSLIB_SERIALIZE(iamdoer, (id)(host)(task_id))
    };

    typedef eosio::multi_index< "iamdoer"_n, iamdoer,
        eosio::indexed_by<"byhost"_n, eosio::const_mem_fun<iamdoer, uint64_t, &iamdoer::byhost>>,
        eosio::indexed_by<"byhosttask"_n, eosio::const_mem_fun<iamdoer, uint128_t, &iamdoer::byhosttask>>
    > iamdoer_index;



/*!
   \brief Структура входящих целей и задач
*/
    struct [[eosio::table]] incoming {
        uint64_t id;
        eosio::name host;
        uint64_t ext_goal_id;
        uint64_t ext_task_id;
        uint64_t my_goal_id;
        uint64_t my_badge_id;
        
        uint64_t primary_key() const {return id;}
        uint64_t byexpr() const {return expiration.sec_since_epoch();}

        EOSLIB_SERIALIZE(incoming, (id)(host)(ext_goal_id)(ext_task_id)(my_goal_id)(my_badge_id))
    };

    typedef eosio::multi_index<"incoming"_n, incoming,
       eosio::indexed_by< "byexpr"_n, eosio::const_mem_fun<incoming, uint64_t, 
                      &incoming::byexpr>>
    > incoming_index;
