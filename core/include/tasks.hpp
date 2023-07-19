/*!
   \brief Структура задач хоста Двойной Спирали.
*/
	struct [[eosio::table, eosio::contract("unicore")]] tasks {
		uint64_t id;
        uint64_t parent_id;
        
		uint64_t goal_id;
        eosio::name host;
        eosio::name creator;
        eosio::name benefactor;
        eosio::name suggester;
        std::string permlink;
        eosio::name type;
        eosio::name status;
		uint64_t priority = 1;
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
		
        bool with_badge;
		uint64_t badge_id;
        bool validated = false;
		bool completed = false;
		bool active = true;
        eosio::time_point_sec created_at;
        eosio::time_point_sec start_at;
		eosio::time_point_sec expired_at;
        
        uint64_t duration;
        bool is_encrypted = false;
        std::string public_key;
        uint64_t positive_votes;
        uint64_t negative_votes;
        std::vector<eosio::name> voters;

        std::string meta;


		uint64_t primary_key()const { return id; }
        
        uint64_t bycreator() const {return creator.value;}
        uint64_t bycurator() const {return curator.value;}
        uint64_t bydoer() const {return doer.value;}
        uint64_t bybenefactor() const {return benefactor.value;}
        uint64_t bygoal() const {return goal_id; }
        uint128_t goalandtask() const { return combine_ids(goal_id, id); }
        uint64_t byhost() const {return host.value; }
        uint64_t bytype() const {return type.value; }
        uint64_t bystatus() const {return status.value; }
        uint64_t bypriority() const {return priority; }
        uint64_t byhasbadge() const {return with_badge; }
        uint64_t bybadge() const {return badge_id; }
        uint128_t crewithtask() const { return combine_ids(creator.value, id); }
        uint128_t crewithgoal() const { return combine_ids(creator.value, goal_id); }
        
        uint64_t byvotes() const { 
            return positive_votes;
        }
        uint64_t bynvotes() const{
            return negative_votes;
        }

	    EOSLIB_SERIALIZE( tasks, (id)(parent_id)(goal_id)(host)(creator)(benefactor)(suggester)(permlink)(type)(status)(priority)(is_regular)(calendar)(is_public)(doer)(role)(level)(title)(data)(requested)(funded)(remain)(for_each)(curator)(with_badge)(badge_id)(validated)(completed)(active)(created_at)(start_at)(expired_at)(duration)(is_encrypted)(public_key)(positive_votes)(negative_votes)(voters)(meta))
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
        eosio::indexed_by<"bydoer"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bydoer>>,
        eosio::indexed_by<"byvotes"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::byvotes>>,
        eosio::indexed_by<"bynvotes"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bynvotes>>,
        eosio::indexed_by<"bystatus"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bystatus>>
        // eosio::indexed_by<"bydoer"_n, eosio::const_mem_fun<tasks, uint64_t, &tasks::bydoer>>
    
    > tasks_index;



// /*!
//    \brief Структура отчетов по задачам хоста Двойной Спирали.
// */
//     struct [[eosio::table, eosio::contract("unicore")]] reports {
//     	uint64_t report_id;
//     	uint64_t task_id; 
//     	uint64_t goal_id;
//         uint64_t type;
//         uint64_t count;
//     	eosio::name username;
//     	eosio::name curator;
//     	eosio::string data;
//     	eosio::asset requested;
//         eosio::asset balance;
//         eosio::asset withdrawed;
//     	bool need_check = true;
//     	bool approved = false;
//     	eosio::string comment;
//         eosio::time_point_sec created_at;
//         eosio::time_point_sec expired_at;

    	
//         uint64_t primary_key() const {return report_id;}
// 		uint64_t byusername() const {return username.value;}
//         uint128_t userwithgoal() const { return combine_ids(username.value, goal_id); }
//         uint128_t userwithtask() const { return combine_ids(username.value, task_id); }
        

//     	EOSLIB_SERIALIZE(reports, (report_id)(task_id)(goal_id)(type)(count)(username)(curator)(data)(requested)(balance)(withdrawed)(need_check)(approved)(comment)(created_at)(expired_at))
//     };

//     typedef eosio::multi_index< "reports"_n, reports,
//         eosio::indexed_by<"userwithtask"_n, eosio::const_mem_fun<reports, uint128_t, &reports::userwithtask>>,
//         eosio::indexed_by<"userwithgoal"_n, eosio::const_mem_fun<reports, uint128_t, &reports::userwithgoal>>,
//         eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<reports, uint64_t, &reports::byusername>>        
        
//     > reports_index;



/*!
   \brief Структура отчетов по задачам хоста Двойной Спирали.
*/
    struct [[eosio::table, eosio::contract("unicore")]] reports3 {
        uint64_t report_id;
        eosio::name status;
        uint64_t task_id; 
        uint64_t goal_id;
        uint64_t type;
        uint64_t count;
        uint64_t duration_secs;
        eosio::asset asset_per_hour;
        eosio::name username;
        eosio::name curator;
        eosio::string data;
        eosio::asset requested;
        eosio::asset balance;
        eosio::asset withdrawed;
        uint64_t power_balance;
        uint64_t withdrawed_power;
        bool need_check = true;
        bool approved = false;
        bool distributed = false;
        eosio::string comment;
        eosio::time_point_sec created_at;
        eosio::time_point_sec expired_at;


        uint64_t positive_votes;
        uint64_t negative_votes;
        std::vector<eosio::name> voters;

        uint64_t primary_key() const {return report_id;}
        uint64_t byusername() const {return username.value;}
        uint64_t bytaskid() const {return task_id;}
        uint64_t bygoalid() const {return goal_id;}
        uint64_t byvotes() const {return positive_votes;}
        uint64_t bynvotes() const {return negative_votes;}

        uint128_t userwithgoal() const { return combine_ids(username.value, goal_id); }
        uint128_t userwithtask() const { return combine_ids(username.value, task_id); }
        

        EOSLIB_SERIALIZE(reports3, (report_id)(status)(task_id)(goal_id)(type)(count)(duration_secs)(asset_per_hour)(username)(curator)(data)(requested)(balance)(withdrawed)(power_balance)(withdrawed_power)(need_check)(approved)(distributed)(comment)(created_at)(expired_at)(positive_votes)(negative_votes)(voters))
    };

    typedef eosio::multi_index< "reports3"_n, reports3,
        eosio::indexed_by<"userwithtask"_n, eosio::const_mem_fun<reports3, uint128_t, &reports3::userwithtask>>,
        eosio::indexed_by<"userwithgoal"_n, eosio::const_mem_fun<reports3, uint128_t, &reports3::userwithgoal>>,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<reports3, uint64_t, &reports3::byusername>>,        
        eosio::indexed_by<"bytaskid"_n, eosio::const_mem_fun<reports3, uint64_t, &reports3::bytaskid>>,
        eosio::indexed_by<"bygoalid"_n, eosio::const_mem_fun<reports3, uint64_t, &reports3::bygoalid>>,
        eosio::indexed_by<"byvotes"_n, eosio::const_mem_fun<reports3, uint64_t, &reports3::byvotes>>
        
    > reports_index;



/*!
   \brief Структура статистики интеллектуальной собственности пользователя
*/
    
struct [[eosio::table, eosio::contract("unicore")]] intelown {
    eosio::name username;
    uint64_t total_reports;
    uint64_t approved_reports;
    
    uint64_t primary_key() const {return username.value;}
    

    EOSLIB_SERIALIZE(intelown, (username)(total_reports)(approved_reports))        

};

    typedef eosio::multi_index<"intelown"_n, intelown> intelown_index;



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


// /*!
//    \brief Структура задач, где аккаунт деятель.
// */
//     struct [[eosio::table, eosio::contract("unicore")]] iamdoer {
//         uint64_t id;
//         eosio::name host;
//         uint64_t task_id; 
        
//         uint64_t primary_key() const {return id;}
//         uint64_t byhost() const {return host.value;}
//         uint128_t byhosttask() const { return combine_ids(host.value, task_id); }

//         EOSLIB_SERIALIZE(iamdoer, (id)(host)(task_id))
//     };

//     typedef eosio::multi_index< "iamdoer"_n, iamdoer,
//         eosio::indexed_by<"byhost"_n, eosio::const_mem_fun<iamdoer, uint64_t, &iamdoer::byhost>>,
//         eosio::indexed_by<"byhosttask"_n, eosio::const_mem_fun<iamdoer, uint128_t, &iamdoer::byhosttask>>
//     > iamdoer_index;



/*!
   \brief Структура входящих целей и задач
*/
    struct [[eosio::table, eosio::contract("unicore")]] incoming {
        uint64_t id;
        eosio::name host;
        uint64_t goal_id;
        uint64_t task_id;
        bool with_badge = false;
        uint64_t my_goal_id;
        uint64_t my_badge_id;
        
        uint64_t primary_key() const {return id;}

        uint128_t byhosttask() const { return combine_ids(host.value, task_id); }
        uint128_t byhostgoal() const { return combine_ids(host.value, goal_id); }
        uint64_t bymygoal() const { return my_goal_id; }

        EOSLIB_SERIALIZE(incoming, (id)(host)(goal_id)(task_id)(with_badge)(my_goal_id)(my_badge_id))
    };

    typedef eosio::multi_index<"incoming"_n, incoming,
       eosio::indexed_by< "byhosttask"_n, eosio::const_mem_fun<incoming, uint128_t, &incoming::byhosttask>>,
       eosio::indexed_by< "byhostgoal"_n, eosio::const_mem_fun<incoming, uint128_t, &incoming::byhostgoal>>,
       eosio::indexed_by< "bymygoal"_n, eosio::const_mem_fun<incoming, uint64_t, &incoming::bymygoal>>
    > incoming_index;
