#include "include/core.hpp"


#include "src/shares.cpp"
#include "src/hosts.cpp"
#include "src/ref.cpp"
#include "src/core.cpp"
#include "src/goals.cpp"
#include "src/voting.cpp"
#include "src/badges.cpp"
#include "src/tasks.cpp"
#include "src/ipfs.cpp"
#include "src/cms.cpp"
#include "src/conditions.cpp"
#include "src/dacs.cpp"
#include "src/products.cpp"

using namespace eosio;


extern "C" {

   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (action == "transfer"_n.value){

            struct transfer{
                eosio::name from;
                eosio::name to;
                eosio::asset quantity;
                std::string memo;
            };

            auto op = eosio::unpack_action_data<transfer>();

            // auto op = eosio::unpack_action_data<eosio::token::transfer>();
            if (op.from != _me){
                auto subcode = op.memo.substr(0,3);
                auto delimeter = op.memo[3];
                if (delimeter == '-'){

                    auto parameter = op.memo.substr(4, op.memo.length());
                    uint64_t subintcode = atoll(subcode.c_str());
                    
                    switch (subintcode){

                        case DEPOSIT: {
                            eosio::name host; 
                            std::string message = "";

                            auto delimeter2 = parameter.find('-');
                    
                            if (delimeter2 != -1){
                                auto host_string = op.memo.substr(4, delimeter2);
                                
                                host = name(host_string.c_str());
                                
                                message = parameter.substr(delimeter2+1, parameter.length());
                        
                            } else {
                                auto host_string = op.memo.substr(4, parameter.length());
                                host = name(host_string.c_str());
                            }
                            require_auth(op.from);
                            unicore::deposit(op.from, host, op.quantity, name(code), message);
                            break;
                        }
                        
                        case BUY_BALANCE: {
                            //check for code inside
                            //Donation for goal
                            auto delimeter2 = parameter.find('-');
                            std::string parameter2 = parameter.substr(delimeter2+1, parameter.length());
                            
                            auto host = name(parameter2.c_str());
                            uint64_t balance_id = atoll(parameter.c_str()); 
                            require_auth(op.from);

                            unicore::buybalance(op.from, host, balance_id, op.quantity, name(code));
                            break;
                        }

                        case DEPOSIT_FOR_SOMEONE: {
                            eosio::name host; 
                            std::string message = "";

                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            auto username = name(username_string.c_str());
                            require_auth(op.from);
                            unicore::deposit(username, host, op.quantity, name(code), message);
                            break;
                        }

                        case PAY_FOR_HOST: {
                            //check for code outside
                            //auto cd = name(code.c_str());
                            //Pay for upgrade
                            
                            auto host = name(parameter.c_str());
                            unicore::pay_for_upgrade(host, op.quantity, name(code));
                            break;
                        }
                        case BUY_SHARES: {
                            //check for code outside
                            //auto cd = name(code.c_str());
                            //Buy Shares
                            auto host = name(parameter.c_str());
                            unicore::buyshares_action(op.from, host, op.quantity, name(code), false);
                            break;
                        }
                        case DONATE_TO_GOAL: {
                            //check for code inside
                            //Donation for goal
                            auto delimeter2 = parameter.find('-');
                            std::string parameter2 = parameter.substr(delimeter2+1, parameter.length());
                            
                            auto host = name(parameter2.c_str());
                            uint64_t goal_id = atoll(parameter.c_str()); 
                            require_auth(op.from);

                            unicore::donate_action(op.from, host, goal_id, op.quantity, name(code));
                            break;
                        }
                        case FUND_EMISSION_POOL: {
                            //direct fund emission pool
                            
                            auto host = name(parameter.c_str());
                            unicore::fund_emi_pool(host, op.quantity, name(code));
                            break;
                        }

                        case BURN_QUANTS: {
                            //BURN QUANTS
                            
                            require_auth(op.from);
                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            eosio::name host = name(host_string.c_str());
                            
                            auto status_string = parameter.substr(delimeter2+1, parameter.length());
                            eosio::name status = name(status_string.c_str());

                            
                            unicore::burn_action(op.from, host, op.quantity, name(code), status);

                            break;
                        };
                        case SUBSCRIBE_AS_ADVISER: {
                            require_auth(op.from);
                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            eosio::name host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            eosio::name username = name(username_string.c_str());

                            
                            unicore::subscribe_action(username, host, op.quantity, name(code), "adviser"_n);

                            break;
                        };

                        case SUBSCRIBE_AS_ASSISTANT: {
                            require_auth(op.from);
                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            eosio::name host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            eosio::name username = name(username_string.c_str());

                            
                            unicore::subscribe_action(username, host, op.quantity, name(code), "assistant"_n);

                            break;
                        };

                        case SPREAD_TO_REFS: {
                            require_auth(op.from);

                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            auto host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            auto username = name(username_string.c_str());

                            account_index accounts(_me, _me.value);
                            auto acc = accounts.find(host.value);
                            sincome_index sincome(_me, host.value);
        
                            auto sinc = sincome.find(acc -> current_pool_id);
                            
                            sincome.modify(sinc, _me, [&](auto &s){
                                s.paid_to_refs += op.quantity;
                            });

                            unicore::spread_to_refs(host, username, op.quantity, op.quantity, name(code));

                            break;
                        }
                        case SPREAD_TO_FUNDS: {
                            require_auth(op.from);

                            auto delimeter2 = parameter.find('-');
                            
                            eosio::name host;
                            std::string message = "";
                    
                            if (delimeter2 != -1){
                                auto host_string = op.memo.substr(4, delimeter2);
                                
                                host = name(host_string.c_str());
                                
                                message = parameter.substr(delimeter2+1, parameter.length());
                        
                            } else {
                                auto host_string = op.memo.substr(4, parameter.length());
                                host = name(host_string.c_str());
                            }

                            unicore::spread_to_funds(name(code), host, op.quantity, message);

                            break;
                        }
                        case SPREAD_TO_DACS: {
                            require_auth(op.from);

                            auto delimeter2 = parameter.find('-');
                    
                            eosio::name host;
                           
                            std::string message = "";
                    
                            if (delimeter2 != -1){
                                auto host_string = op.memo.substr(4, delimeter2);
                                
                                host = name(host_string.c_str());
                                
                                message = parameter.substr(delimeter2+1, parameter.length());
                        
                            } else {
                                auto host_string = op.memo.substr(4, parameter.length());
                                host = name(host_string.c_str());
                            }


                            unicore::push_to_dacs_funds(host, op.quantity, name(code), message);

                            

                            break;
                        }
                        case MAKE_FREE_VESTING: {
                            require_auth(op.from);
                            auto delimeter2 = parameter.find('-');
                            std::string parameter2 = parameter.substr(delimeter2+1, parameter.length());
                            
                            auto owner = name(parameter2.c_str());
                            uint64_t seconds = atoll(parameter.c_str()); 
                            
                            unicore::make_vesting_action(owner, ""_n, name(code), op.quantity, seconds, "freevesting"_n);
      
                            break;
                        }
                        case ADD_INTERNAL_BALANCE: {
                            
                            unicore::add_balance(op.from, op.quantity, eosio::name(code));  
                            
                            break;
                        }

                        // default:
                            // break;
                           
                    }
                }
                
            }
        } else if (code == _me.value){

            switch (action) {

                EOSIO_DISPATCH_HELPER(unicore, (init)(exittail)(sellbalance)(cancelsellba)
                    (changemode)(refrollback)
                    //GOALS
                    (setgoal)(editgoal)(setgcreator)(gaccept)(setbenefac)(gpause)
                    (paydebt)(delgoal)(report)(check)
                    //CMS
                    (setcontent)(rmcontent)(setcmsconfig)(checkstatus)
                    //VOTING
                    (vote)(rvote)
                    //SHARES
                    (sellshares)(refreshsh)(withdrawsh)(withpbenefit)(withrbenefit)
                    (withrbalance)(cancrefwithd)(complrefwith)(setwithdrwal)(refreshpu)
                    //HOSTS
                    (upgrade)(setconsensus)(compensator)(setlevels)(cchildhost)(setwebsite)
                    (setahost)(closeahost)(openahost)(rmahost)(enpmarket)(emittomarket)
                    (emitquote)(emitquants)(emitpower2)(dispmarket)(settiming)(setflows)
                    //CORE
                    (setparams)(start)(setstartdate)(refreshbal)(setbalmeta)(refreshst)(withdraw)
                    (withdrawsold)(priorenter)(gwithdraw)(setemi)(edithost)(setcondition)
                    //BADGES
                    (setbadge)(delbadge)(giftbadge)(backbadge)
                    //TASKS
                    (settask)(deltask)(setinctask)(tactivate)(tcomplete)(tdeactivate)(setpgoal)
                    (setdoer)(validate)(setpriority)(jointask)(canceljtask)(settaskmeta)(setreport)
                    (delreport)(withdrawrepo)(distrepo)(editreport)(approver)(disapprover)(fundtask)
                    (setarch)(dfundgoal)(fundchildgoa)
                    //VACS
                    (addvac)(rmvac)(addvprop)(rmvprop)(approvevac)(apprvprop)
                    //DACS
                    (adddac)(rmdac)(withdrdacinc)(fixs)(spreadlist)
                    //PRODUCTS
                    (createprod)(editprod)(buysyssubscr)(addflow)(buysecret)(deriveprod)
                    (addpermiss)(rmpermiss)(payvirtual)
                )
             
            }
            
        }
}

}; 
