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
                    
                    //codes:
                    //100 - deposit
                    //110 - pay for host
                    //200 - buyshares
                    //300 - goal activate action
                    //400 - direct fund emission pool
                    //500 - buy data
                    //600 - direct buy quants
                    //666 - direct fund commquanty fund balance for some purposes
                    
                    switch (subintcode){

                        case 100: {
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
                        
                        case 150: {
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

                        case 101: {
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

                        case 110: {
                            //check for code outside
                            //auto cd = name(code.c_str());
                            //Pay for upgrade
                            
                            auto host = name(parameter.c_str());
                            unicore::pay_for_upgrade(host, op.quantity, name(code));
                            break;
                        }
                        case 200: {
                            //check for code outside
                            //auto cd = name(code.c_str());
                            //Buy Shares
                            auto host = name(parameter.c_str());
                            unicore::buyshares_action(op.from, host, op.quantity, name(code), false);
                            break;
                        }
                        case 300: {
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
                        case 400: {
                            //direct fund emission pool
                            
                            auto host = name(parameter.c_str());
                            unicore::fund_emi_pool(host, op.quantity, name(code));
                            break;
                        }

                        // case 500: {
                        //     //BUY DATA
                        //     auto delimeter2 = parameter.find('-');
                        //     std::string parameter2 = parameter.substr(delimeter2+1, parameter.length());
                            
                        //     auto owner = name(parameter2.c_str());
                        //     auto buyer = op.from;
                        //     uint64_t data_id = atoll(parameter.c_str());  
                        //     require_auth(buyer);  

                        //     unicore::buydata_action(owner, data_id, buyer, op.quantity, name(code));
                        //     break;
                        // }
                        case 600: {
                            //BUY QUANTS
                            //direct buy saled quants
                            require_auth(op.from);

                            auto host = name(parameter.c_str());
                            
                            unicore::buy_action(op.from, host, op.quantity, name(code), true, true, true);

                            break;
                        }

                        case 666:{
                            // execute_action(name(receiver), name(code), &unicore::createfund);

                            auto delimeter2 = parameter.find('-');
                            std::string parameter2 = parameter.substr(delimeter2+1, parameter.length());
                            
                            auto descriptor = parameter2.c_str();
                            

                            unicore::createfund(name(code), op.quantity, descriptor);

                            // fcore().createfund_action(eosio::unpack_action_data<createfund>());
                            break;
                        };
                    
                        case 650: {
                            //TODO check guest status and if guest - pay
                            require_auth(_gateway);
                            
                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            auto host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            auto username = name(username_string.c_str());

                            unicore::buy_account(username, host, op.quantity, name(code), "participant"_n);

                            break;
                        };
                        case 660: {
                            //TODO check guest status and if guest - pay
                            require_auth(_gateway);
                            
                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            auto host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            auto username = name(username_string.c_str());

                            unicore::buy_account(username, host, op.quantity, name(code), "partner"_n);

                            break;
                        };
                        case 670: {
                            //TODO check guest status and if guest - pay
                            require_auth(_gateway);
                            
                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            auto host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            auto username = name(username_string.c_str());

                            unicore::buy_account(username, host, op.quantity, name(code), "business"_n);

                            break;
                        };
                        case 800: {
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
                        case 801: {
                            //BURN QUANTS
                            //status 
                            // adviser
                            require_auth(op.from);
                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            eosio::name host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            eosio::name username = name(username_string.c_str());

                            
                            unicore::subscribe_action(username, host, op.quantity, name(code), "adviser"_n);

                            break;
                        };

                        case 802: {
                            //BURN QUANTS
                            //status 
                            // assistant
                            require_auth(op.from);
                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            eosio::name host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            eosio::name username = name(username_string.c_str());

                            
                            unicore::subscribe_action(username, host, op.quantity, name(code), "assistant"_n);

                            break;
                        };

                        case 111: {
                            //SPREAD
                            
                            //
                            require_auth(op.from);

                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            auto host = name(host_string.c_str());
                            
                            auto username_string = parameter.substr(delimeter2+1, parameter.length());
                            auto username = name(username_string.c_str());

                            print("ON SPREAD");
                            unicore::spread_to_refs(host, username, op.quantity, op.quantity, name(code));

                            break;
                        }
                        case 112: {
                            //SPREAD
                            
                            //
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

                            
                            print("ON SPREAD");
                            unicore::spread_to_funds(name(code), host, op.quantity, message);

                            break;
                        }
                        case 222: {
                            //SPREAD
                            
                            //
                            require_auth(op.from);

                            auto delimeter2 = parameter.find('-');
                    
                            auto host_string = op.memo.substr(4, delimeter2);
                            
                            auto host = name(host_string.c_str());

                            print("ON SPREAD to DAC");
                            unicore::spread_to_dacs(host, op.quantity, name(code));

                            break;
                        }
                        case 699: {
                            require_auth(op.from);
                            auto delimeter2 = parameter.find('-');
                            std::string parameter2 = parameter.substr(delimeter2+1, parameter.length());
                            
                            auto owner = name(parameter2.c_str());
                            uint64_t seconds = atoll(parameter.c_str()); 
                            
                            unicore::make_vesting_action(owner, ""_n, name(code), op.quantity, seconds, "freevesting"_n);
      
                            break;
                        }
                        case 700: {
                            
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
                 case "init"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::init);
                    break;
                 }
                 case "exittail"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::exittail);
                    break;
                 }
                 case "sellbalance"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::sellbalance);
                    break;
                 }
                 case "cancelsellba"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::cancelsellba);
                    break;
                 }
                 case "changemode"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::changemode);
                    break;
                 }

                //GOALS
                 case "setgoal"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setgoal);
                    break;
                 }
                 case "editgoal"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::editgoal);
                    break;
                 }
                 case "setgcreator"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setgcreator);
                    break;
                 }
                 case "gaccept"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::gaccept);
                    break;
                 }
                case "setbenefac"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setbenefac);
                    break;
                 }
                 case "refrollback"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::refrollback);
                    break;
                 }
                 case "gpause"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::gpause);
                    break;
                 }
                 
                 case "paydebt"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::paydebt);
                    break;
                 }
                 case "delgoal"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::delgoal);
                    break;
                 }
                 case "gsponsor"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::gsponsor);
                    break;
                 }
                 case "report"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::report);
                    break;
                 }
                 case "check"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::check);
                    break;
                }

                //CMS
                case "setcontent"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setcontent);
                    break;
                }

                case "rmcontent"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::rmcontent);
                    break;
                }

                case "setcmsconfig"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setcmsconfig);
                    break;
                }
                case "checkstatus"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::checkstatus);
                    break;
                }
                
                // //IPFS
                // case "setstorage"_n.value: {
                //     execute_action(name(receiver), name(code), &unicore::setstorage);
                //     // ipfs().setstorage_action(eosio::unpack_action_data<setstorage>());
                //     break;
                // }

                // case "removeroute"_n.value: {
                //     execute_action(name(receiver), name(code), &unicore::removeroute);
                //     // ipfs().removeroute_action(eosio::unpack_action_data<removeroute>());
                //     break;
                // }

                // case "setipfskey"_n.value: {
                //     execute_action(name(receiver), name(code), &unicore::setipfskey);
                //     // ipfs().setipfskey_action(eosio::unpack_action_data<setipfskey>());
                //     break;
                // }

                // case "selldata"_n.value: {
                //     execute_action(name(receiver), name(code), &unicore::selldata);
                //     // ipfs().selldata_action(eosio::unpack_action_data<selldata>());
                //     break;
                // }
                
                // case "dataapprove"_n.value: {
                //     execute_action(name(receiver), name(code), &unicore::dataapprove);
                //     // ipfs().orbapprove_action(eosio::unpack_action_data<dataapprove>());
                //     break;
                // }
               
                //VOTING
                 case "vote"_n.value: { 
                    execute_action(name(receiver), name(code), &unicore::vote);
                    // voting().vote_action(eosio::unpack_action_data<vote>());
                    break;
                 }

                 case "rvote"_n.value: { 
                    execute_action(name(receiver), name(code), &unicore::rvote);
                    // voting().vote_action(eosio::unpack_action_data<vote>());
                    break;
                 }

                 // case "setliqpower"_n.value: {
                 //    execute_action(name(receiver), name(code), &unicore::setliqpower);
                 //    break;
                 // }
                 // case "incrusersegm"_n.value: {
                 //    execute_action(name(receiver), name(code), &unicore::incrusersegm);
                 //    break;
                 // }

                //SHARES
                case "sellshares"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::sellshares);
                    break;
                };
                case "delshares"_n.value: {
                    //?????????????????

                    struct delsharesstruct {
                        eosio::name from; 
                        eosio::name reciever; 
                        eosio::name host;
                        uint64_t shares;
                    };

                    auto op = eosio::unpack_action_data<delsharesstruct>();
                    
                    require_auth(op.from);

                    unicore::delegate_shares_action(op.from, op.reciever, op.host, op.shares);

                    break;
                };
                case "undelshares"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::undelshares);
                    break;
                };
                case "refreshsh"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::refreshsh);
                    break;
                };

                case "withdrawsh"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withdrawsh);
                    break;
                };
                case "withpbenefit"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withpbenefit);
                    break;
                };
                case "withrbenefit"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withrbenefit);
                    break;
                };
                case "withrbalance"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withrbalance);
                    break;  
                };
                case "cancrefwithd"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::cancrefwithd);
                    break;  
                };
                case "complrefwith"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::complrefwith);
                    break;  
                };

                case "setwithdrwal"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setwithdrwal);
                    break;  
                };

                case "refreshpu"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::refreshpu);
                    break;
                };

                //HOSTS
                case "upgrade"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::upgrade);
                    break;
                };
                case "setconsensus"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setconsensus);
                    break;
                };
                case "compensator"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::compensator);
                    break;
                };
                case "setlevels"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setlevels);
                    break;
                };
                case "cchildhost"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::cchildhost);
                    break;
                };
                case "setwebsite"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setwebsite);
                    break;
                };
                case "setahost"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setahost);
                    break;
                };
                case "closeahost"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::closeahost);
                    break;  
                };
                case "openahost"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::openahost);
                    break;
                };
                case "rmahost"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::rmahost);
                    break;
                };
                case "enpmarket"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::enpmarket);
                    break;
                };
                case "emittomarket"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::emittomarket);
                    break;
                };
                case "emitquote"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::emitquote);
                    break;
                };    
                case "emitpower"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::emitpower);
                    break;
                };
                case "emitpower2"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::emitpower2);
                    break;
                };
                case "dispmarket"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::dispmarket);
                    break;
                };

                case "settiming"_n.value: {
                  execute_action(name(receiver), name(code), &unicore::settiming);
                  break;  
                };

                case "setflows"_n.value: {
                  execute_action(name(receiver), name(code), &unicore::setflows);
                  break;  
                };

                //CORE
                case "setparams"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setparams);
                    break;
                };
                case "start"_n.value: {
                   execute_action(name(receiver), name(code), &unicore::start);
                   break;
                };
                case "setstartdate"_n.value: {
                   execute_action(name(receiver), name(code), &unicore::setstartdate);
                   break;
                };
                case "refreshbal"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::refreshbal);
                    break;
                };
                case "refreshst"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::refreshst);
                    // break;
                    // struct refreshstruct {
                    //     eosio::name host;
                    // };
                    
                    // auto op = eosio::unpack_action_data<refreshstruct>();
                    
                    // unicore::refresh_state(op.host);
                    break;
                };
                
                case "withdraw"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withdraw);
                    break;
                };
                
                case "withdrawsold"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withdrawsold);
                    break;
                };
                
                case "priorenter"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::priorenter);
                    break;
                };
                case "gwithdraw"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::gwithdraw);
                    break;
                };
                case "setemi"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::setemi);
                    break;
                };

                case "edithost"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::edithost);
                    break;
                };

                case "setcondition"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::setcondition);
                    break;
                };

                //POT
                case "addhostofund"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::addhostofund);
                    break;
                };
                case "enablesale"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::enablesale);
                    break;
                };
                case "rmhosfrfund"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::rmhosfrfund);
                    break;
                };
                
                case "transfromgf"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::transfromgf);
                    break;
                };
            
                //BADGES
                case "setbadge"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::setbadge);
                    break;
                }
                case "delbadge"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::delbadge);
                    break;
                }
                case "giftbadge"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::giftbadge);
                    break;
                }
                case "backbadge"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::backbadge);
                    break;
                }

                //TASKS

                case "settask"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::settask);
                    break;
                }
                case "deltask"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::deltask);
                    break;
                }
                case "setinctask"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::setinctask);
                    break;
                }
                case "tactivate"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::tactivate);
                    break;
                }
                case "tcomplete"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::tcomplete);
                    break;
                }

                case "tdeactivate"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::tdeactivate);
                    break;
                }
                case "setpgoal"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setpgoal);
                    break;
                }
                case "setdoer"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setdoer);
                    break;
                }
                case "validate"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::validate);
                    break;
                }
                case "setpriority"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setpriority);
                    break;
                }
                case "jointask"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::jointask);
                    break;
                }
                case "canceljtask"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::canceljtask);
                    break;
                }
                case "settaskmeta"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::settaskmeta);
                    break;
                }

                case "setreport"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::setreport);
                    break;
                }
                case "delreport"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::delreport);
                    break;
                }
                case "withdrawrepo"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withdrawrepo);
                    break;
                }
                case "distrepo"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::distrepo);
                    break;
                }
                case "editreport"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::editreport);
                    break;
                }

                case "approver"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::approver);
                    break;
                }

                case "disapprover"_n.value:{
                    execute_action(name(receiver), name(code), &unicore::disapprover);
                    break;
                }

                case "fundtask"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::fundtask);
                    break;
                }

                case "setarch"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::setarch);
                    break;
                }

                case "dfundgoal"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::dfundgoal);
                    break;
                }
                case "fundchildgoa"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::fundchildgoa);
                    break;
                }
                case "convert"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::convert);
                    break;
                }

                //VACS
                case "addvac"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::addvac);
                    break;
                }
                case "rmvac"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::rmvac);
                    break;
                }
                case "addvprop"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::addvprop);
                    break;
                }
                case "rmvprop"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::rmvprop);
                    break;
                }
                case "approvevac"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::approvevac);
                    break;
                }
                case "apprvprop"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::apprvprop);
                    break;
                }

                //DACS
                case "adddac"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::adddac);
                    break;
                }
                case "rmdac"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::rmdac);
                    break;
                }
                case "withdrdacinc"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withdrdacinc);
                    break;
                }

                //BENEFACTORS
                case "addben"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::addben);
                    break;
                }
                case "rmben"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::rmben);
                    break;
                }
                
                case "withdrbeninc"_n.value: {
                    execute_action(name(receiver), name(code), &unicore::withdrbeninc);
                    break;
                }

                case "fixs"_n.value : {
                    execute_action(name(receiver), name(code), &unicore::fixs);
                    break;
                }
            }
            
        }
}

}; 
