
/*!
   \brief Структура целей хоста Двойной Спирали.
*/

    struct [[eosio::table, eosio::contract("unicore")]]  goals {
        uint64_t id;
        uint64_t parent_id;
        eosio::name type;
        eosio::name creator;

        eosio::name benefactor;
        eosio::name host;
        
        eosio::name status;
        eosio::name who_can_create_tasks;
        
        uint64_t benefactors_weight;
        
        uint64_t duration;
        uint64_t priority = 1;
        
        std::string title;
        std::string description;
        
        eosio::asset target;
        
        eosio::asset target1;
        eosio::asset target2;
        
        
        eosio::asset available;
        eosio::asset withdrawed;
        
        uint64_t debt_count;
        eosio::asset debt_amount;

        uint64_t positive_votes;
        uint64_t negative_votes;
        uint64_t filled_votes;
        uint64_t total_tasks;
        uint64_t total_reports;
        uint64_t approved_reports;
        
        bool is_encrypted = false;
        std::string public_key;

        eosio::time_point_sec created;
        eosio::time_point_sec start_at;
        eosio::time_point_sec finish_at;
        eosio::time_point_sec expired_at;

        std::vector<eosio::name> voters;        

        std::string report;
        std::string meta;

        uint64_t second_circuit_votes;
        eosio::asset total_asset_on_distribution;
        eosio::asset remain_asset_on_distribution;
        
        uint64_t total_power_on_distribution;
        uint64_t remain_power_on_distribution;

        uint64_t cashback;
        eosio::asset refs_amount;


        
        uint64_t primary_key()const { return id; }
      
        uint64_t byvotes() const { 
            return positive_votes;
        }

        uint64_t bynvotes() const { 
            return negative_votes;
        }

        // double byvotes() const { 
        //     return (double)total_votes; 
        // }
        
        // uint64_t byvotes() const { 
        //     return -uint64_t(uint128_t(total_votes) + pow(2, 63));
        // }

        uint64_t bytype() const {return type.value;}
        uint64_t bystatus() const {return status.value;}
        uint64_t byparentid() const {return parent_id;}

        
        uint64_t bycreated() const {return created.sec_since_epoch(); }
        uint64_t bypriority() const {return priority; }
        uint64_t byusername() const {return creator.value; }
        uint64_t byhost() const {return host.value;}
        
        uint128_t by_username_and_host() const { return combine_ids(creator.value, host.value); }

        
        EOSLIB_SERIALIZE( goals, (id)(parent_id)(type)(creator)(benefactor)(host)(status)(who_can_create_tasks)(benefactors_weight)(duration)(priority)(title)(description)(target)(target1)(target2)(available)(withdrawed)(debt_count)(debt_amount)(positive_votes)(negative_votes)(filled_votes)(total_tasks)
            (total_reports)(approved_reports)(is_encrypted)(public_key)(created)(start_at)(finish_at)(expired_at)(voters)(report)(meta)
            (second_circuit_votes)(total_asset_on_distribution)(remain_asset_on_distribution)(total_power_on_distribution)(remain_power_on_distribution)(cashback)(refs_amount))
    };

    typedef eosio::multi_index <"goals"_n, goals,
        eosio::indexed_by<"byhost"_n, eosio::const_mem_fun<goals, uint64_t, &goals::byhost>>,
        eosio::indexed_by<"bystatus"_n, eosio::const_mem_fun<goals, uint64_t, &goals::bystatus>>,
        eosio::indexed_by<"type"_n, eosio::const_mem_fun<goals, uint64_t, &goals::bytype>>,
        eosio::indexed_by<"votes"_n, eosio::const_mem_fun<goals, uint64_t, &goals::byvotes>>,
        eosio::indexed_by<"nvotes"_n, eosio::const_mem_fun<goals, uint64_t, &goals::bynvotes>>,
        eosio::indexed_by<"byparentid"_n, eosio::const_mem_fun<goals, uint64_t, &goals::byparentid>>,
        eosio::indexed_by<"creator"_n, eosio::const_mem_fun<goals, uint64_t, &goals::byusername>>,
        eosio::indexed_by<"created"_n, eosio::const_mem_fun<goals, uint64_t, &goals::bycreated>>,
        eosio::indexed_by<"bypriority"_n, eosio::const_mem_fun<goals, uint64_t, &goals::bypriority>>
    > goals_index;




/*!
   \brief Структура участников цели хоста Двойной Спирали
*/

    struct [[eosio::table, eosio::contract("unicore")]]  goalspartic {
        uint64_t id;
        uint64_t goal_id;
        eosio::name username;
        eosio::name role;
        bool completed = false;
        eosio::time_point_sec expiration;
        
        uint64_t primary_key()const { return id; }
        uint128_t byusergoal() const { return combine_ids(username.value, goal_id); }
        
        uint64_t byusername()const {return username.value;}
        uint64_t byrole() const {return role.value;}

        EOSLIB_SERIALIZE( goalspartic, (id)(goal_id)(username)(role)(completed)(expiration))

    };

    typedef eosio::multi_index <"goalspartic"_n, goalspartic,
        eosio::indexed_by<"byusergoal"_n, eosio::const_mem_fun<goalspartic, uint128_t, &goalspartic::byusergoal>>,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<goalspartic, uint64_t, &goalspartic::byusername>>,
        eosio::indexed_by<"byrole"_n, eosio::const_mem_fun<goalspartic, uint64_t, &goalspartic::byrole>>
    > goalspartic_index;


/*!
   \brief Структура бенефакторов цели хоста Двойной Спирали
*/

    struct [[eosio::table, eosio::contract("unicore")]] benefactors {
        uint64_t id;
        uint64_t goal_id;
        eosio::name benefactor;
        uint64_t weight;
        eosio::asset income;
        uint128_t income_in_segments;
        eosio::asset withdrawed;
        eosio::asset income_limit;
        uint64_t primary_key() const {return id;}  
        uint64_t bygoalid() const {return goal_id;}

        uint128_t bybengoal() const { return combine_ids(benefactor.value, goal_id); }
        
        EOSLIB_SERIALIZE(benefactors, (id)(goal_id)(benefactor)(weight)(income)(income_in_segments)(withdrawed)(income_limit))      
    };

    typedef eosio::multi_index <"benefactors"_n, benefactors,
    eosio::indexed_by<"bygoalid"_n, eosio::const_mem_fun<benefactors, uint64_t, &benefactors::bygoalid>>,
    eosio::indexed_by<"bybengoal"_n, eosio::const_mem_fun<benefactors, uint128_t, &benefactors::bybengoal>>
    > benefactors_index;


