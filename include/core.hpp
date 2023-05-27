#pragma once

#include <algorithm>
#include <cmath>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/contract.hpp>
#include <eosio/action.hpp>
#include <eosio/system.hpp>
#include "eosio.token.hpp"
#include <eosio/print.hpp>
#include <eosio/datastream.hpp>


#include "hosts.hpp"
#include "ref.hpp"
#include "shares.hpp"
#include "goals.hpp"
#include "voting.hpp"
#include "badges.hpp"
#include "tasks.hpp"
#include "ipfs.hpp"
#include "cms.hpp"
#include "crypto.hpp"
#include "conditions.hpp"
#include "products.hpp"
#include "consts.hpp"

/**
\defgroup public_consts CONSTS
\brief Константы контракта
*/

/**
\defgroup public_actions ACTIONS
\brief Методы действий контракта
*/

/**
\defgroup public_tables TABLES
\brief Структуры таблиц контракта
*/

/**
\defgroup public_subcodes SUBCODES
\brief Субкоды входящих переводов
*/

/**
\defgroup public_modules MODULES
\brief Подмодули
*/

// namespace eosio {


class [[eosio::contract]] unicore : public eosio::contract {
    public:
        unicore(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds ): eosio::contract(receiver, code, ds)
        {}
        void apply(uint64_t receiver, uint64_t code, uint64_t action);
        // uint128_t unicore::combine_ids(const uint64_t &x, const uint64_t &y);
        // uint128_t unicore::combine_ids2(const uint64_t &x, const uint64_t &y);

        static void cut_tail(uint64_t current_pool_id, eosio::name host);
        
        [[eosio::action]] void sellbalance(eosio::name host, eosio::name username, uint64_t balance_id);
        [[eosio::action]] void cancelsellba(eosio::name host, eosio::name username, uint64_t balance_id);

        static void buybalance(eosio::name username, eosio::name host, uint64_t balance_id, eosio::asset amount, eosio::name contract);

        [[eosio::action]] void exittail(eosio::name username, eosio::name host, uint64_t id);

        [[eosio::action]] void changemode(eosio::name host, eosio::name mode);

        
        [[eosio::action]] void setparams(eosio::name host, eosio::name chost, uint64_t size_of_pool,
            uint64_t quants_precision, uint64_t overlap, uint64_t profit_growth, uint64_t base_rate,
            uint64_t loss_percent, uint64_t compensator_percent, uint64_t pool_limit, uint64_t pool_timeout, uint64_t priority_seconds);

        [[eosio::action]] void start(eosio::name host, eosio::name chost); 
        [[eosio::action]] void withdraw(eosio::name username, eosio::name host, uint64_t balance_id);
        [[eosio::action]] void withdrawsold(eosio::name username, eosio::name host, uint64_t balance_id);

        [[eosio::action]] void priorenter(eosio::name username, eosio::name host, uint64_t balance_id);
        [[eosio::action]] void refreshbal(eosio::name username, eosio::name host, uint64_t balance_id, uint64_t partrefresh);
        
        [[eosio::action]] void setstartdate(eosio::name host, eosio::time_point_sec start_at); 
        [[eosio::action]] void checkstatus(eosio::name host, eosio::name username);

        static bool get_status_expiration(eosio::name host, eosio::name username);

        static void pay_for_upgrade(eosio::name username, eosio::asset amount, eosio::name code);
        
        static void refresh_state(eosio::name host);

        [[eosio::action]] void init(uint64_t system_income);

        [[eosio::action]] void refreshst(eosio::name username, eosio::name host);

        //BADGES
        [[eosio::action]] void setbadge(uint64_t id, eosio::name host, eosio::string caption, eosio::string description, eosio::string iurl, eosio::string pic, uint64_t total, uint64_t power);
        [[eosio::action]] void giftbadge(eosio::name host, eosio::name to, uint64_t badge_id, eosio::string comment, bool netted, uint64_t goal_id, uint64_t task_id);
        [[eosio::action]] void backbadge(eosio::name host, eosio::name from, uint64_t usbadge_id, eosio::string comment);
        [[eosio::action]] void delbadge(eosio::name host, uint64_t badge_id);

        static void giftbadge_action(eosio::name host, eosio::name to, uint64_t badge_id, eosio::string comment, bool netted, bool own, uint64_t goal_id, uint64_t task_id);
        static void deposit ( eosio::name username, eosio::name host, eosio::asset amount, eosio::name code, std::string message );
        [[eosio::action]] void settcrbadge(eosio::name host, uint64_t badge_id);

        //CMS
        [[eosio::action]] void setcontent(eosio::name username, eosio::name type, eosio::name lang, eosio::string content);
        [[eosio::action]] void rmcontent(eosio::name username, eosio::name type);
        [[eosio::action]] void setcmsconfig(eosio::name username, eosio::string config);

        //GOALS
        [[eosio::action]] void setgoal(eosio::name creator, eosio::name host, uint64_t parent_id, std::string title, std::string description, eosio::asset target, std::string meta);
        [[eosio::action]] void editgoal(eosio::name editor, eosio::name host, uint64_t goal_id, std::string title, std::string description, std::string meta);



        [[eosio::action]] void dfundgoal(eosio::name architect, eosio::name host, uint64_t goal_id, eosio::asset amount, std::string comment);
        [[eosio::action]] void fundchildgoa(eosio::name architect, eosio::name host, uint64_t goal_id, eosio::asset amount);
        
        [[eosio::action]] void setgcreator(eosio::name oldcreator, eosio::name newcreator, eosio::name host, uint64_t goal_id);
        
        [[eosio::action]] void gaccept(eosio::name host, uint64_t goal_id, uint64_t parent_goal_id, bool status);
        [[eosio::action]] void gpause(eosio::name host, uint64_t goal_id);
        [[eosio::action]] void setbenefac(eosio::name host, uint64_t goal_id, eosio::name benefactor);
  
        [[eosio::action]] void delgoal(eosio::name username, eosio::name host, uint64_t goal_id);
        [[eosio::action]] void report(eosio::name username, eosio::name host, uint64_t goal_id, std::string report);
        [[eosio::action]] void check(eosio::name architect, eosio::name host, uint64_t goal_id);
        [[eosio::action]] void gwithdraw(eosio::name username, eosio::name host, uint64_t goal_id);
        [[eosio::action]] void setemi(eosio::name host, uint64_t percent, uint64_t gtop);
        static void donate_action(eosio::name from, eosio::name host, uint64_t goal_id, eosio::asset quantity, eosio::name code);
        static eosio::asset adjust_goals_emission_pool(eosio::name hostname, eosio::asset host_income);
        static void fund_emi_pool ( eosio::name host, eosio::asset amount, eosio::name code );
        
        static eosio::asset emit(eosio::name host, eosio::asset host_income, eosio::asset max_income);
        
        static uint64_t get_status_level(eosio::name host, eosio::name username);
        
        static void burn_action(eosio::name username, eosio::name host, eosio::asset quantity, eosio::name code, eosio::name status);
        static void subscribe_action(eosio::name username, eosio::name host, eosio::asset quantity, eosio::name code, eosio::name status);

        
        //HOST
        [[eosio::action]] void setarch(eosio::name host, eosio::name architect);
        [[eosio::action]] void setconsensus(eosio::name host, uint64_t consensus_percent, bool voting_only_up);
        
        [[eosio::action]] void upgrade(eosio::name username, eosio::name platform, std::string title, std::string purpose, uint64_t total_shares, eosio::asset quote_amount, eosio::name quote_token_contract, eosio::asset root_token, eosio::name root_token_contract, bool voting_only_up, uint64_t consensus_percent, uint64_t referral_percent, uint64_t dacs_percent, uint64_t cfund_percent, uint64_t hfund_percent, std::vector<uint64_t> levels, uint64_t emission_percent, uint64_t gtop, std::string meta);
        [[eosio::action]] void cchildhost(eosio::name parent_host, eosio::name chost);
        [[eosio::action]] void compensator(eosio::name host, uint64_t compensator_percent);
        

        [[eosio::action]] void edithost(eosio::name architect, eosio::name host, eosio::name platform, eosio::string title, eosio::string purpose, eosio::string manifest, eosio::name power_market_id, eosio::string meta);
        [[eosio::action]] void fixs(eosio::name host, eosio::name username);
        [[eosio::action]] void setlevels(eosio::name host, std::vector<uint64_t> levels);


        [[eosio::action]] void settiming(eosio::name host, uint64_t pool_timeout, uint64_t priority_seconds);
        [[eosio::action]] void setflows(eosio::name host, uint64_t ref_percent, uint64_t dacs_percent, uint64_t cfund_percent, uint64_t hfund_percent);

        [[eosio::action]] void refrollback(eosio::name host, eosio::name username, uint64_t balance_id);
        

        //BENEFACTORS
        static void spread_to_benefactors(eosio::name host, eosio::asset amount, uint64_t goal_id);
        
        [[eosio::action]] void addvac(uint64_t id, bool is_edit, eosio::name creator, eosio::name host, eosio::name limit_type, eosio::asset income_limit, uint64_t weight, std::string title, std::string descriptor) ;
        [[eosio::action]] void rmvac(eosio::name host, uint64_t id);
        [[eosio::action]] void addvprop(uint64_t id, bool is_edit, eosio::name creator, eosio::name host, uint64_t vac_id, uint64_t weight, std::string why_me, std::string contacts);
        [[eosio::action]] void rmvprop(eosio::name host, uint64_t vprop_id);
        [[eosio::action]] void approvevac(eosio::name host, uint64_t vac_id);
        [[eosio::action]] void apprvprop(eosio::name host, uint64_t vprop_id);

        //DACS
        [[eosio::action]] void adddac(eosio::name username, eosio::name host, uint64_t weight, eosio::name limit_type, eosio::asset income_limit, std::string title, std::string descriptor);
        [[eosio::action]] void rmdac(eosio::name username, eosio::name host);

        // [[eosio::action]] void editrole(eosio::name username, eosio::name host, uint64_t weight);
        // [[eosio::action]] void rmrole(eosio::name username, eosio::name host, uint64_t weight);

        static void spread_to_dacs(eosio::name host, eosio::asset amount, eosio::name contract);

        static void spread_to_funds(eosio::name code, eosio::name host, eosio::asset quantity, std::string comment);

        static void spread_to_refs(eosio::name host, eosio::name username, eosio::asset spread_amount, eosio::asset from_amount, eosio::name token_contract);

        static void log_event_with_shares (eosio::name username, eosio::name host, int64_t new_power);

        [[eosio::action]] void withdrdacinc(eosio::name username, eosio::name host);
        [[eosio::action]] void setwebsite(eosio::name host, eosio::name ahostname, eosio::string website, eosio::name type, std::string meta);

        [[eosio::action]] void rmahost(eosio::name host, eosio::name ahostname);
        [[eosio::action]] void setahost(eosio::name host, eosio::name ahostname);
        [[eosio::action]] void closeahost(eosio::name host);
        [[eosio::action]] void openahost(eosio::name host);

        static void add_user_stat(eosio::name type, eosio::name username, eosio::name contract, eosio::asset nominal_amount, eosio::asset withdraw_amount);
        static void add_ref_stat(eosio::name username, eosio::name contract, eosio::asset withdrawed);
        static void add_host_stat(eosio::name type, eosio::name username, eosio::name host, eosio::asset amount);
        
        static void add_core_stat(eosio::name type, eosio::name host, eosio::asset amount);
        static void add_host_stat2(eosio::name type, eosio::name username, eosio::name host, eosio::asset amount);
        
        [[eosio::action]] void withrbalance(eosio::name username, eosio::name host);
        [[eosio::action]] void setwithdrwal(eosio::name username, eosio::name host, eosio::string wallet);

        [[eosio::action]] void cancrefwithd(eosio::name host, uint64_t id, eosio::string comment);
        [[eosio::action]] void complrefwith(eosio::name host, uint64_t id, eosio::string comment);

        [[eosio::action]] void dispmarket(eosio::name host);
        [[eosio::action]] void enpmarket(eosio::name host);

        [[eosio::action]] void emitquote(eosio::name host, eosio::asset amount, eosio::name contract);
        [[eosio::action]] void emitpower(eosio::name host, eosio::name username, int64_t user_shares, bool is_change);
        [[eosio::action]] void emitpower2(eosio::name host, uint64_t goal_id, uint64_t shares);
        [[eosio::action]] void emittomarket(eosio::name host, eosio::name username, uint64_t user_shares, eosio::asset amount);
        [[eosio::action]] void withrbenefit(eosio::name username, eosio::name host, uint64_t id);
        [[eosio::action]] void withpbenefit(eosio::name username, eosio::name host, uint64_t log_id);
        [[eosio::action]] void refreshpu(eosio::name username, eosio::name host, uint64_t log_id);
        [[eosio::action]] void refreshsh (eosio::name owner, uint64_t id);
        [[eosio::action]] void withdrawsh(eosio::name owner, uint64_t id);
        [[eosio::action]] void sellshares(eosio::name username, eosio::name host, uint64_t shares);
        static uint64_t buyshares_action ( eosio::name buyer, eosio::name host, eosio::asset amount, eosio::name code, bool is_frozen );
        static void delegate_shares_action(eosio::name from, eosio::name reciever, eosio::name host, uint64_t shares);
        

        //TASKS
        [[eosio::action]] void settask(eosio::name host, eosio::name creator, std::string permlink, uint64_t goal_id, uint64_t priority, eosio::string title, eosio::string data, eosio::asset requested, bool is_public, eosio::name doer, eosio::asset for_each, bool with_badge, uint64_t badge_id, bool is_batch, uint64_t parent_batch_id, bool is_regular, std::vector<uint64_t> calendar, eosio::time_point_sec start_at,eosio::time_point_sec expired_at, std::string meta);
        [[eosio::action]] void fundtask(eosio::name host, uint64_t task_id, eosio::asset amount, eosio::string comment);
        [[eosio::action]] void tactivate(eosio::name host, uint64_t task_id);
        [[eosio::action]] void tdeactivate(eosio::name host, uint64_t task_id);

        [[eosio::action]] void tcomplete(eosio::name host, uint64_t task_id);

        [[eosio::action]] void setpgoal(eosio::name host, uint64_t task_id, uint64_t goal_id);
        [[eosio::action]] void setdoer(eosio::name host, uint64_t task_id, eosio::name doer);
        
        [[eosio::action]] void setpriority(eosio::name host, uint64_t task_id, uint64_t priority);

        [[eosio::action]] void validate(eosio::name host, uint64_t task_id, uint64_t goal_id, eosio::name doer);
        [[eosio::action]] void jointask(eosio::name host, uint64_t task_id, eosio::name doer, std::string comment);
        [[eosio::action]] void canceljtask(eosio::name host, uint64_t task_id, eosio::name doer);

        [[eosio::action]] void settaskmeta(eosio::name host, uint64_t task_id, std::string meta);
        
        static void setincoming(eosio::name doer, eosio::name host, uint64_t goal_id, uint64_t task_id);
        static void delincoming(eosio::name doer, eosio::name host, uint64_t goal_id, uint64_t task_id);


        static void check_and_gift_netted_badge(eosio::name username, eosio::name host, uint64_t task_id);
        [[eosio::action]] void setinctask(eosio::name username, uint64_t income_id, bool with_badge, uint64_t my_goal_id, uint64_t my_badge_id);



        [[eosio::action]] void deltask(eosio::name host, uint64_t task_id);
        [[eosio::action]] void paydebt(eosio::name host, uint64_t goal_id);

        [[eosio::action]] void setreport(eosio::name host, eosio::name username, uint64_t task_id, eosio::string data, uint64_t duration_secs, eosio::asset asset_per_hour);
        [[eosio::action]] void editreport(eosio::name host, eosio::name username, uint64_t report_id, eosio::string data);
        [[eosio::action]] void approver(eosio::name host, uint64_t report_id, eosio::string comment); 
        [[eosio::action]] void disapprover(eosio::name host, uint64_t report_id, eosio::string comment);
        [[eosio::action]] void delreport(eosio::name host, eosio::name username, uint64_t report_id);

        [[eosio::action]] void withdrawrepo(eosio::name username, eosio::name host, uint64_t report_id);
        [[eosio::action]] void distrepo(eosio::name username, eosio::name host, uint64_t report_id);


        //PRODUCTS
        [[eosio::action]] void createprod(name host, eosio::name type, std::string title, std::string description, std::string encrypted_data, std::string public_key, eosio::name token_contract, asset price, uint64_t duration);
        [[eosio::action]] void editprod(eosio::name host, eosio::name type, uint64_t product_id, std::string title, std::string description, std::string encrypted_data, std::string public_key, asset price, uint64_t duration);
        [[eosio::action]] void buyproduct(eosio::name buyer, eosio::name host, uint64_t product_id, uint64_t quantity);

        static void add_vbalance(eosio::name username, eosio::name host, eosio::asset quantity);
        static void sub_vbalance(eosio::name username, eosio::name host, eosio::asset quantity);
        static bool get_product_expiration(eosio::name host, eosio::name username, uint64_t product_id);



        //VOTING
        [[eosio::action]] void vote(eosio::name voter, eosio::name host, uint64_t goal_id, bool up);
        [[eosio::action]] void rvote(eosio::name voter, eosio::name host, uint64_t report_id, bool up);

        static void propagate_votes_changes(eosio::name host, eosio::name voter, uint64_t old_power, uint64_t new_power);
        
        //CONDITIONS
        [[eosio::action]] void setcondition(eosio::name host, eosio::string key, uint64_t value);
        [[eosio::action]] void rmcondition(eosio::name host, uint64_t key); 
        static void rmfromhostwl(eosio::name host, eosio::name username);
        static void check_status(eosio::name host, eosio::name username, eosio::asset amount, eosio::name status);
        static void check_subscribe(eosio::name host, eosio::name username, eosio::asset amount, eosio::name status);

        static void add_balance(eosio::name payer, eosio::asset quantity, eosio::name contract);   
        static void sub_balance(eosio::name username, eosio::asset quantity, eosio::name contract);
    

        static uint64_t getcondition(eosio::name host, eosio::string key);

        static void addtohostwl(eosio::name host, eosio::name username);
        static bool checkcondition(eosio::name host, eosio::string key, uint64_t value);
        static void checkminpwr(eosio::name host, eosio::name username);

        std::string symbol_to_string(eosio::asset val) const;
        static void change_bw_trade_graph(eosio::name host, uint64_t pool_id, uint64_t cycle_num, uint64_t pool_num, uint64_t buy_rate, uint64_t next_buy_rate, uint64_t total_quants, uint64_t remain_quants, std::string color);
        
        static void add_coredhistory(eosio::name host, eosio::name username, uint64_t pool_id, eosio::asset amount, std::string action, std::string message);

        static void make_vesting_action(eosio::name owner, eosio::name host, eosio::name contract, eosio::asset amount, uint64_t vesting_seconds, eosio::name type);
        static void create_bancor_market(eosio::name name, eosio::name host, uint64_t total_shares, eosio::asset quote_amount, eosio::name quote_token_contract, uint64_t vesting_seconds);
        static std::vector <eosio::asset> calculate_forecast(eosio::name username, eosio::name host, uint64_t quants, uint64_t pool_num, eosio::asset compensator_amount, eosio::asset purchase_amount, bool calculate_first, bool calculate_zero);
        static void fill_pool(eosio::name username, eosio::name host, uint64_t quants, eosio::asset amount, uint64_t filled_pool_id);
        static void give_shares_with_badge_action (eosio::name host, eosio::name reciever, uint64_t shares);
        static void back_shares_with_badge_action (eosio::name host, eosio::name from, uint64_t shares);
        
        static uint64_t get_global_id(eosio::name key);

        static void optimize_parameters_of_new_cycle (eosio::name host, eosio::name main_host);
        static void emplace_first_pools(eosio::name parent_host, eosio::name main_host, eosio::symbol root_symbol, eosio::time_point_sec start_at);



};

    
    /**
     * @brief      Таблица промежуточного хранения балансов пользователей.
     * @ingroup public_tables
     * @table balance
     * @contract _me
     * @scope username
     * @details Таблица баланса пользователя пополняется им путём совершения перевода на аккаунт контракта p2p. При создании ордера используется баланс пользователя из этой таблицы. Чтобы исключить необходимость пользователю контролировать свой баланс в контракте p2p, терминал доступа вызывает транзакцию с одновременно двумя действиями: перевод на аккаунт p2p и создание ордера на ту же сумму. 
     */

    struct [[eosio::table, eosio::contract("unicore")]] ubalance {
        uint64_t id;                    /*!< идентификатор баланса */
        eosio::name contract;           /*!< имя контракта токена */
        eosio::asset quantity;          /*!< количество токенов на балансе */

        uint64_t primary_key() const {return id;} /*!< return id - primary_key */
        uint128_t byconsym() const {return combine_ids(contract.value, quantity.symbol.code().raw());} /*!< return combine_ids(contract, quantity) - комбинированный secondary_index 2 */

        EOSLIB_SERIALIZE(ubalance, (id)(contract)(quantity))
    };


    typedef eosio::multi_index<"ubalance"_n, ubalance,
    
      eosio::indexed_by< "byconsym"_n, eosio::const_mem_fun<ubalance, uint128_t, &ubalance::byconsym>>
    
    > ubalances_index;




    /**
     * @brief      Таблица хранения счётчиков
     * @contract _me
     * @scope nft | _me
     * @table counts
     * @ingroup public_tables
     * @details Таблица хранит категории событий хоста. 
     */
    struct [[eosio::table, eosio::contract("unicore")]] counts {
      eosio::name key;
      eosio::name secondary_key;
      uint64_t value;
      double   double_value;

      uint64_t primary_key() const {return key.value;}       /*!< return id - primary_key */
      uint128_t keyskey() const {return combine_ids(key.value, secondary_key.value);} /*!< (contract, blocked_now.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */
      uint128_t keyvalue() const {return combine_ids(key.value, value);} /*!< (contract, blocked_now.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */
      

      EOSLIB_SERIALIZE(counts, (key)(secondary_key)(value)(double_value))
    };

    typedef eosio::multi_index< "counts"_n, counts,
        eosio::indexed_by<"keyskey"_n, eosio::const_mem_fun<counts, uint128_t, &counts::keyskey>>,
        eosio::indexed_by<"keyvalue"_n, eosio::const_mem_fun<counts, uint128_t, &counts::keyvalue>>
    > counts_index;


// }


/*!
   \brief Структура системного процента, изымаего протокол из обращения при каждом полу-обороте Двойной Спирали каждого хоста.
*/

    struct [[eosio::table, eosio::contract("unicore")]] gpercents {
        eosio::name key;
        uint64_t value;
        uint64_t primary_key() const {return key.value;}

        EOSLIB_SERIALIZE(gpercents, (key)(value))
    };

    typedef eosio::multi_index<"gpercents"_n, gpercents> gpercents_index;

/*!
   \brief Структура балансов пользователя у всех хостов Двойной Спирали
*/
    
    struct [[eosio::table, eosio::contract("unicore")]] balance {
        uint64_t id;
        eosio::name owner;
        eosio::name host;
        eosio::name chost;
        eosio::name status = "process"_n;
        uint64_t cycle_num;
        uint64_t pool_num;
        uint64_t global_pool_id;
        uint64_t quants_for_sale;
        uint64_t next_quants_for_sale;
        uint64_t last_recalculated_pool_id = 1;
        bool win = false; //true if win, false if lose or nominal
        int64_t root_percent;
        int64_t convert_percent;
        std::string pool_color;
        eosio::asset available; 
        eosio::asset purchase_amount;
        eosio::asset compensator_amount;
        eosio::asset start_convert_amount;
        eosio::asset if_convert; 
        eosio::asset if_convert_to_power;
        eosio::asset solded_for;
        eosio::asset withdrawed;
        std::vector<eosio::asset> forecasts;
        eosio::asset ref_amount; 
        eosio::asset dac_amount;
        eosio::asset cfund_amount;
        eosio::asset hfund_amount;
        eosio::asset sys_amount;

        eosio::string meta; 

        uint64_t primary_key() const {return id;}
        uint64_t byowner() const {return owner.value;}
        uint64_t byavailable() const {return available.amount;}
        uint64_t bystatus() const {return status.value;}


        EOSLIB_SERIALIZE(balance, (id)(owner)(host)(chost)(status)(cycle_num)(pool_num)(global_pool_id)(quants_for_sale)(next_quants_for_sale)(last_recalculated_pool_id)(win)(root_percent)(convert_percent)(pool_color)(available)(purchase_amount)(compensator_amount)(start_convert_amount)(if_convert)(if_convert_to_power)(solded_for)(withdrawed)(forecasts)(ref_amount)(dac_amount)(cfund_amount)(hfund_amount)(sys_amount)(meta))
    
        eosio::name get_ahost() const {
            if (host == chost)
                return host;
            else
                return chost;
        }
    };

    typedef eosio::multi_index<"balance"_n, balance,
        eosio::indexed_by<"byowner"_n, eosio::const_mem_fun<balance, uint64_t, &balance::byowner>>,
        eosio::indexed_by<"byavailable"_n, eosio::const_mem_fun<balance, uint64_t, &balance::byavailable>>,
        eosio::indexed_by<"bystatus"_n, eosio::const_mem_fun<balance, uint64_t, &balance::bystatus>>
    > balance_index;



/*!
   \brief Структура статистики балансов пользователя у всех хостов Двойной Спирали
*/
    
struct [[eosio::table, eosio::contract("unicore")]] userstat {
    uint64_t id;
    eosio::name username;
    eosio::name contract;

    std::string symbol;
    uint64_t precision;

    eosio::asset blocked_now;
    eosio::asset total_nominal;
    eosio::asset total_withdraw;
    
    uint64_t primary_key() const {return id;}
    uint64_t byusername() const {return username.value;}
    uint128_t byconuser() const {return combine_ids(contract.value, username.value);} /*!< (contract, blocked_now.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */

    uint128_t byusernom() const {return combine_ids(username.value, -total_nominal.amount);} /*!< (contract, blocked_now.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */
    uint128_t byuserblock() const {return combine_ids(username.value, -blocked_now.amount);} /*!< (contract, blocked_now.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */


    EOSLIB_SERIALIZE(userstat, (id)(username)(contract)(symbol)(precision)(blocked_now)(total_nominal)(total_withdraw))        

};

    typedef eosio::multi_index<"userstat"_n, userstat,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<userstat, uint64_t, &userstat::byusername>>,
        eosio::indexed_by<"byconuser"_n, eosio::const_mem_fun<userstat, uint128_t, &userstat::byconuser>>,
        eosio::indexed_by<"byusernom"_n, eosio::const_mem_fun<userstat, uint128_t, &userstat::byusernom>>,
        eosio::indexed_by<"byuserblock"_n, eosio::const_mem_fun<userstat, uint128_t, &userstat::byuserblock>>
    
    > userstat_index;



/*!
   \brief Структура статистики балансов пользователя у одного хоста Двойной Спирали
*/
    
struct [[eosio::table, eosio::contract("unicore")]] hoststat {
    eosio::name username;
    eosio::asset blocked_now;
    
    uint64_t primary_key() const {return username.value;}
    
    uint128_t byuserblock() const {return combine_ids(username.value, -blocked_now.amount);} /*!< (contract, blocked_now.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */


    EOSLIB_SERIALIZE(hoststat, (username)(blocked_now))        

};

    typedef eosio::multi_index<"hoststat"_n, hoststat,
        eosio::indexed_by<"byuserblock"_n, eosio::const_mem_fun<hoststat, uint128_t, &hoststat::byuserblock>>
    > hoststat_index;



/*!
   \brief Структура статистики балансов пользователя у одного хоста Двойной Спирали
*/
    
struct [[eosio::table, eosio::contract("unicore")]] hoststat2 {
    eosio::name username;
    eosio::asset volume;
    
    uint64_t primary_key() const {return username.value;}
    

    EOSLIB_SERIALIZE(hoststat2, (username)(volume))        

};

    typedef eosio::multi_index<"hoststat2"_n, hoststat2> hoststat_index2;


/*!
   \brief Структура статистики балансов пользователя у одного хоста Двойной Спирали
*/
    
struct [[eosio::table, eosio::contract("unicore")]] corestat {
    eosio::name hostname;
    eosio::asset volume;
    
    uint64_t primary_key() const {return hostname.value;}
    
    uint64_t byvolume() const {return volume.amount;} 


    EOSLIB_SERIALIZE(corestat, (hostname)(volume))        

};

    typedef eosio::multi_index<"corestat"_n, corestat,
        eosio::indexed_by<"byvolume"_n, eosio::const_mem_fun<corestat, uint64_t, &corestat::byvolume>>
    > corestat_index;




/*!
   \brief Структура статистики балансов пользователя у всех хостов Двойной Спирали
*/
    
struct [[eosio::table, eosio::contract("unicore")]] refstat {
    uint64_t id;
    eosio::name username;
    eosio::name contract;

    std::string symbol;
    uint64_t precision;

    eosio::asset withdrawed;
    
    uint64_t primary_key() const {return id;}
    uint64_t byusername() const {return username.value;}
    uint128_t byconuser() const {return combine_ids(contract.value, username.value);} /*!< (contract, blocked_now.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */

    uint128_t byuserwith() const {return combine_ids(username.value, -withdrawed.amount);} /*!< (contract, blocked_now.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */
    

    EOSLIB_SERIALIZE(refstat, (id)(username)(contract)(symbol)(precision)(withdrawed))        

};

    typedef eosio::multi_index<"refstat"_n, refstat,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<refstat, uint64_t, &refstat::byusername>>,
        eosio::indexed_by<"byconuser"_n, eosio::const_mem_fun<refstat, uint128_t, &refstat::byconuser>>,
        eosio::indexed_by<"byuserwith"_n, eosio::const_mem_fun<refstat, uint128_t, &refstat::byuserwith>>
        
    > refstat_index;


/*!
   \brief Структура для отображения Двойной Спирали в виде торгового графика типа BLACK-AND-WHITE.
*/

    struct [[eosio::table, eosio::contract("unicore")]] bwtradegraph{
        uint64_t pool_id;
        uint64_t cycle_num;
        uint64_t pool_num;
        uint64_t open;
        uint64_t high;
        uint64_t low;
        uint64_t close;
        bool is_white;
        uint64_t primary_key() const {return pool_id;}
        uint64_t bycycle() const {return cycle_num;}

        EOSLIB_SERIALIZE(bwtradegraph, (pool_id)(cycle_num)(pool_num)(open)(high)(low)(close)(is_white))        
    };

    typedef eosio::multi_index<"bwtradegraph"_n, bwtradegraph,
        eosio::indexed_by<"bycycle"_n, eosio::const_mem_fun<bwtradegraph, uint64_t, &bwtradegraph::bycycle>>
    > bwtradegraph_index;

    


/*!
   \brief Структура для учёта развития циклов хоста Двойной Спирали.
*/

    struct [[eosio::table, eosio::contract("unicore")]] cycle{
        uint64_t id;
        eosio::name ahost;
        uint64_t start_at_global_pool_id;
        uint64_t finish_at_global_pool_id;
        eosio::asset emitted;
        uint64_t primary_key() const {return id;}

        EOSLIB_SERIALIZE(cycle, (id)(ahost)(start_at_global_pool_id)(finish_at_global_pool_id)(emitted));
    };

    typedef eosio::multi_index<"cycle"_n, cycle> cycle_index;
    


/*!
   \brief Структура для учёта распределения бассейнов внутренней учетной единицы хоста Двойной Спирали.
*/
    struct [[eosio::table, eosio::contract("unicore")]] pool {
        uint64_t id;
        eosio::name ahost;
        uint64_t cycle_num;
        uint64_t pool_num;
        std::string color;
        uint64_t total_quants; 
        uint64_t remain_quants;
        uint64_t creserved_quants; 
        eosio::asset filled;
        eosio::asset remain;
        uint64_t filled_percent;
        eosio::asset pool_cost;
        eosio::asset quant_cost;
        eosio::asset total_win_withdraw;
        eosio::asset total_loss_withdraw;
        eosio::time_point_sec pool_started_at;
        eosio::time_point_sec priority_until;
        eosio::time_point_sec pool_expired_at;
        
        uint64_t primary_key() const {return id;}
        uint64_t bycycle() const {return cycle_num;}
        
        EOSLIB_SERIALIZE(pool,(id)(ahost)(cycle_num)(pool_num)(color)(total_quants)(remain_quants)(creserved_quants)(filled)(remain)(filled_percent)(pool_cost)(quant_cost)(total_win_withdraw)(total_loss_withdraw)(pool_started_at)(priority_until)(pool_expired_at))
    };

    typedef eosio::multi_index<"pool"_n, pool> pool_index;
    

/*!
   \brief Структура для хранения сообщений в режиме чата ограниченной длинны от спонсоров хоста Двойной Спирали.
*/
    struct [[eosio::table, eosio::contract("unicore")]] coredhistory{
        uint64_t id;
        uint64_t pool_id;
        uint64_t pool_num;
        uint64_t cycle_num;
        std::string color;
        eosio::name username;
        std::string action;
        std::string message;
        eosio::asset amount;
        uint64_t primary_key() const {return id;}

        EOSLIB_SERIALIZE(coredhistory, (id)(pool_id)(pool_num)(cycle_num)(color)(username)(action)(message)(amount));
    
    };
    
    typedef eosio::multi_index<"coredhistory"_n, coredhistory> coredhistory_index;
    


/*!
   \brief Структура для хранения живой очереди приоритетных участников
*/
    struct [[eosio::table, eosio::contract("unicore")]] tail {
        uint64_t id;
        
        eosio::name username;
        eosio::time_point_sec enter_at;
        eosio::asset amount;
        uint64_t exit_pool_id;

        uint64_t primary_key() const {return id;}
        uint64_t byenter() const {return enter_at.sec_since_epoch();}
        uint64_t byamount() const {return amount.amount;}

        uint64_t byusername() const {return username.value;}


        EOSLIB_SERIALIZE(tail, (id)(username)(enter_at)(amount)(exit_pool_id));
    
    };

    
    typedef eosio::multi_index<"tail"_n, tail,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<tail, uint64_t, &tail::byusername>>,
        eosio::indexed_by<"byexpr"_n, eosio::const_mem_fun<tail, uint64_t, &tail::byenter>>,
        eosio::indexed_by<"byamount"_n, eosio::const_mem_fun<tail, uint64_t, &tail::byamount>>
    > tail_index;
    

/*!
   \brief Структура учёта системного дохода фондов хоста Двойной Спирали.
*/
    struct [[eosio::table, eosio::contract("unicore")]] sincome {
        uint64_t pool_id;
        eosio::name ahost;
        uint64_t cycle_num;
        uint64_t pool_num;
        uint64_t liquid_power;
        eosio::asset max;
        eosio::asset total;
        eosio::asset paid_to_refs;
        eosio::asset paid_to_dacs;
        eosio::asset paid_to_cfund;
        eosio::asset paid_to_hfund;
        eosio::asset paid_to_sys;
        uint128_t hfund_in_segments;
        uint128_t distributed_segments;

        uint64_t primary_key() const {return pool_id;}
        uint128_t cyclandpool() const { return combine_ids(cycle_num, pool_num); }
        
        EOSLIB_SERIALIZE(sincome, (pool_id)(ahost)(cycle_num)(pool_num)(liquid_power)(max)(total)(paid_to_refs)(paid_to_dacs)(paid_to_cfund)(paid_to_hfund)(paid_to_sys)(hfund_in_segments)(distributed_segments))

    };
    typedef eosio::multi_index<"sincome"_n, sincome,
    eosio::indexed_by<"cyclandpool"_n, eosio::const_mem_fun<sincome, uint128_t, &sincome::cyclandpool>>
    > sincome_index;
    

/*!
   \brief Структура хостов, платформ и их сайтов. 
*/

    struct  [[eosio::table, eosio::contract("unicore")]] ahosts{
        eosio::name username;
        eosio::string website;
        eosio::checksum256 hash;
        bool is_host = false;

        eosio::name type;
        uint64_t votes;
        std::string title;
        std::string purpose;
        std::string manifest;
        bool comments_is_enabled = false;
        std::string meta;

        uint64_t primary_key() const{return username.value;}
        uint64_t byvotes() const {return votes;}

        eosio::checksum256 byuuid() const { return hash; }
        
        eosio::checksum256 hashit(std::string str) const
        {
            return eosio::sha256(const_cast<char*>(str.c_str()), str.size());
        }
          

        EOSLIB_SERIALIZE(ahosts, (username)(website)(hash)(is_host)(type)(votes)(title)(purpose)(manifest)(comments_is_enabled)(meta))
    };

    typedef eosio::multi_index<"ahosts"_n, ahosts,
        eosio::indexed_by<"byvotes"_n, eosio::const_mem_fun<ahosts, uint64_t, &ahosts::byvotes>>,
        eosio::indexed_by<"byuuid"_n, eosio::const_mem_fun<ahosts, eosio::checksum256, &ahosts::byuuid>>
    > ahosts_index;


/*!
   \brief Структура целевого долга хоста Двойной Спирали. 
*/

    struct [[eosio::table, eosio::contract("unicore")]] debts {
        uint64_t id;
        uint64_t goal_id;
        eosio::name username;
        eosio::asset amount;
        
        uint64_t primary_key() const {return id;}
        uint64_t bygoal() const {return goal_id;}
        uint64_t byusername() const {return username.value;}

        EOSLIB_SERIALIZE(debts, (id)(goal_id)(username)(amount))
    };

    typedef eosio::multi_index< "debts"_n, debts,
        eosio::indexed_by<"bygoal"_n, eosio::const_mem_fun<debts, uint64_t, &debts::bygoal>>,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<debts, uint64_t, &debts::byusername>>
    > debts_index;



    struct [[eosio::table]] guests {
        eosio::name username;
        
        eosio::name registrator;
        eosio::public_key public_key;
        eosio::asset cpu;
        eosio::asset net;
        bool set_referer = false;
        eosio::time_point_sec expiration;

        eosio::asset to_pay;
        
        uint64_t primary_key() const {return username.value;}
        uint64_t byexpr() const {return expiration.sec_since_epoch();}

        EOSLIB_SERIALIZE(guests, (username)(registrator)(public_key)(cpu)(net)(set_referer)(expiration)(to_pay))
    };

    typedef eosio::multi_index<"guests"_n, guests,
       eosio::indexed_by< "byexpr"_n, eosio::const_mem_fun<guests, uint64_t, 
                      &guests::byexpr>>
    > guests_index;


        /**
     * @brief      Таблица содержит курсы конвертации к доллару.
     * @ingroup public_tables
     * @table usdrates
     * @contract _me
     * @scope _me
     * @details    Курсы обновляются аккаунтом rater методом setrate или системным контрактом eosio методом uprate. 
    */
    
    struct [[eosio::table]] usdrates {
        uint64_t id;                        /*!< идентификатор курса */
        eosio::name out_contract;           /*!< контракт выхода; если в конвертации используется внешняя валюта (например, фиатный RUB), контракт не устанавливается. Во внутренних конвертациях используется только при указании курса жетона ядра системы к доллару. */
        eosio::asset out_asset;             /*!< токен выхода */
        double rate;                        /*!< курс токена выхода к доллару */
        eosio::time_point_sec updated_at;   /*!< дата последнего обновления курса */
        
        uint64_t primary_key() const {return id;} /*!< return id - primary_key */
        uint128_t byconsym() const {return combine_ids(out_contract.value, out_asset.symbol.code().raw());} /*!< (out_contract, out_asset.symbol) - комбинированный secondary_key для получения курса по имени выходного контракта и символу */

    };

    typedef eosio::multi_index<"usdrates"_n, usdrates,
    
      eosio::indexed_by< "byconsym"_n, eosio::const_mem_fun<usdrates, uint128_t, &usdrates::byconsym>>
    
    > usdrates_index;




