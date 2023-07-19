// https://chat.openai.com/share/33082b92-ec10-461b-9e7a-e47b1d23134f
// void unicore::add_member(eosio::name new_member, uint8_t weight) {
//     require_auth(eosio::name("host")); // Замените на имя хоста DAC

//     // Проверка наличия пользователя в DAC
//     dacs_index dacs(_me, _me.value);
//     auto existing = dacs.find(new_member.value);
//     eosio::check(existing == dacs.end(), "User is already a member of the DAC.");

//     // Добавление нового пользователя в DAC
//     dacs.emplace(_me, [&](auto &d) {
//         d.username = new_member;
//         d.weight = weight;
//         // Остальные параметры...
//     });

//     // Получение текущего authority для аккаунта хоста
//     eosio::authority current_auth = eosio::get_auth(eosio::name("host"), eosio::name("active"));

//     // Добавление нового участника в authority аккаунта хоста в сортированном порядке
//     auto it = std::upper_bound(current_auth.accounts.begin(), current_auth.accounts.end(), new_member.to_string(), 
//         [](const std::string &str, const eosio::key_weight &kw) { 
//             return str < kw.key.actor.to_string(); 
//         }
//     );
//     current_auth.accounts.insert(it, {{new_member, eosio::name("active")}, weight});

//     // Обновление threshold authority аккаунта хоста на основе общего веса участников DAC
//     current_auth.threshold += weight;

//     // Обновление authority аккаунта хоста
//     eosio::action(
//         eosio::permission_level{eosio::name("host"), eosio::name("owner")},
//         eosio::name("eosio"),
//         eosio::name("updateauth"),
//         std::make_tuple(eosio::name("host"), eosio::name("active"), eosio::name("owner"), current_auth)
//     ).send();
// }

// void unicore::remove_member(eosio::name member_to_remove) {
//     require_auth(eosio::name("host")); // Замените на имя хоста DAC

//     // Проверка наличия пользователя в DAC
//     dacs_index dacs(_me, _me.value);
//     auto existing = dacs.find(member_to_remove.value);
//     eosio::check(existing != dacs.end(), "User is not a member of the DAC.");

//     // Получение текущего authority для аккаунта хоста
//     eosio::authority current_auth = eosio::get_auth(eosio::name("host"), eosio::name("active"));

//     // Удаление участника из authority аккаунта хоста
//     for (auto it = current_auth.accounts.begin(); it != current_auth.accounts.end(); ++it) {
//         if (it->permission.actor == member_to_remove) {
//             current_auth.threshold -= it->weight;
//             current_auth.accounts.erase(it);
//             break;
//         }
//     }

//     // Обновление authority аккаунта хоста
//     eosio::action(
//         eosio::permission_level{eosio::name("host"), eosio::name("owner")},
//         eosio::name("eosio"),
//         eosio::name("updateauth"),
//         std::make_tuple(eosio::name("host"), eosio::name("active"), eosio::name("owner"), current_auth)
//     ).send();

//     // Удаление участника из DAC
//     dacs.erase(existing);
// }


[[eosio::action]] void unicore::withdrdacinc(eosio::name username, eosio::name host){
    require_auth(username);
    dacs_index dacs(_me, host.value);

    auto dac = dacs.find(username.value);

    eosio::check(dac != dacs.end(), "DAC is not found");
    
    account_index accounts(_me, _me.value);
    auto acc = accounts.find(host.value);

    auto root_symbol = acc->get_root_symbol();
    eosio::asset to_pay = dac -> income;

    if (to_pay.amount > 0) {

        uint64_t vesting_seconds = unicore::getcondition(host, "teamvestsecs");
        
        if (vesting_seconds > 0) {

            make_vesting_action(username, host, acc->root_token_contract, to_pay, vesting_seconds, "teamwithdraw"_n);

        } else {

            action(
                permission_level{ _me, "active"_n },
                acc->root_token_contract, "transfer"_n,
                std::make_tuple( _me, username, to_pay, std::string("DAC income")) 
            ).send();

        }

        dacs.modify(dac, username, [&](auto &d){
            d.income = asset(0, root_symbol);
            d.income_in_segments = 0;
            d.withdrawed += to_pay;
            d.last_pay_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
        });    
    };
    

}




[[eosio::action]] void unicore::adddac(eosio::name username, eosio::name host, uint64_t weight, eosio::name limit_type, eosio::asset income_limit, std::string title, std::string descriptor) {
    
    account_index accounts(_me, _me.value);
    dacs_index dacs(_me, host.value);

    eosio::check( is_account( username ), "user account does not exist");

    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "Host is not found");
    
    require_auth(acc -> architect);
    
    auto root_symbol = acc->get_root_symbol();
    
    auto dac = dacs.find(username.value);
   

    if (dac == dacs.end()){
        dacs.emplace(acc -> architect, [&](auto &d){
            d.dac = username;
            d.weight = weight;
            d.income = asset(0, root_symbol);
            // d.income_limit = asset(0, root_symbol);
            d.withdrawed = asset(0, root_symbol);
            d.income_in_segments = 0;
            d.role = title;
            d.description = descriptor;
            d.limit_type = limit_type;
            d.income_limit = income_limit;
            d.last_pay_at = eosio::time_point_sec(0);
            d.created_at = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());
        });

        accounts.modify(acc, acc -> architect, [&](auto &h){
            h.total_dacs_weight += weight;
        });       

    } else {
        int64_t new_weight = weight - dac->weight;

        dacs.modify(dac, acc -> architect, [&](auto &d){
            d.weight += new_weight;
            d.role = title;
        });

        accounts.modify(acc, acc -> architect, [&](auto &h){
            h.total_dacs_weight += new_weight;
        });        

    }

};

[[eosio::action]] void unicore::rmdac(eosio::name username, eosio::name host) {
    require_auth(host);
    account_index accounts(_me, _me.value);
    dacs_index dacs(_me, host.value);

    auto acc = accounts.find(host.value);
    eosio::check(acc != accounts.end(), "Host is not found");
    auto dac = dacs.find(username.value);
    
    eosio::check(dac != dacs.end(), "DAC is not found");

    accounts.modify(acc, host, [&](auto &h){
        h.total_dacs_weight -= dac->weight;
    });
    
    if (dac->income.amount > 0)
        action(
            permission_level{ _me, "active"_n },
            acc->root_token_contract, "transfer"_n,
            std::make_tuple( _me, username, dac->income, std::string("DAC income before remove")) 
        ).send();

    dacs.erase(dac);
};


