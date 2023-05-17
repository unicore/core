
/**
 * @brief     Модуль CMS
 * Позволяет каждому сообществу использовать веб-конструктор приложений UNI. 
 */

  /**
   * @brief      Метод установки языковых файлов
   *
   * @param[in]  op    The operation
   */
 [[eosio::action]] void unicore::setcontent(eosio::name username, eosio::name type, eosio::name lang , eosio::string content){
    require_auth(username);

    cmscontent_index contents(_me, username.value);
    auto content_exst = contents.find(type.value);
    
    if (content_exst == contents.end()){
      
      contents.emplace(username, [&](auto &c){
        c.type = type;
        c.lang = lang;
        c.content = content;
      });

    } else {
      contents.modify(content_exst, username, [&](auto &c){
        c.lang = lang;
        c.content = content;
      });
    };
  }

  /**
   * @brief      Метод удаления языкового файла
   *
   * @param[in]  op    The operation
   */
  [[eosio::action]] void unicore::rmcontent(eosio::name username, eosio::name type){
    require_auth(username);
    cmscontent_index contents(_me, username.value);
    auto content = contents.find(type.value);
    contents.erase(content);
  }


  /**
   * @brief      Метод установки CMS-конфига 
   *
   * @param[in]  op    The operation
   */
  [[eosio::action]] void unicore::setcmsconfig(eosio::name username, eosio::string config){
    require_auth(username);

    cmsconfig_index cmsconfigs(_me, username.value);
    
    account_index accounts(_me, _me.value);  

    auto acc = accounts.find(username.value);
    // check(acc != accounts.end(), "Host is not found");


    auto config_exist = cmsconfigs.find(1);
    
    if (config_exist == cmsconfigs.end()){
      
      cmsconfigs.emplace(username, [&](auto &c){
        c.id = 1;
        c.config = config;
      });

    } else {
      cmsconfigs.modify(config_exist, username, [&](auto &c){
        c.config = config;
      });
    };
  }


