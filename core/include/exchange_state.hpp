#pragma once


#include <eosio/datastream.hpp>
#include <eosio/serialize.hpp>


   using eosio::asset;
   using eosio::symbol;

   typedef double real_type;
   /*!
       \brief Структура взамодействия с рынками торгового робота Bancor. 
   */

   /**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    */

   struct [[eosio::table, eosio::contract("unicore")]] market {
      eosio::name name;
      
      uint64_t vesting_seconds = 0;
      eosio::asset    supply;

   /*!
       \brief Структура коннектора рынка торгового робота Bancor.
   */      
      struct connector {
         eosio::asset balance;
         double weight = 1;
         eosio::name contract;
         
         EOSLIB_SERIALIZE( connector, (balance)(weight)(contract))
      };

      connector base;
      connector quote;

      uint64_t primary_key()const { return name.value; }
      
      asset convert_to_exchange( connector& reserve, const asset& payment );
      asset convert_from_exchange( connector& reserve, const asset& tokens );
      asset convert( const asset& from, const symbol& to );

      asset direct_convert( const asset& from, const symbol& to );

      static int64_t get_bancor_output( int64_t inp_reserve,
                                        int64_t out_reserve,
                                        int64_t inp );
      static int64_t get_bancor_input( int64_t out_reserve,
                                       int64_t inp_reserve,
                                       int64_t out );

      EOSLIB_SERIALIZE( market, (name)(vesting_seconds)(supply)(base)(quote) )
   };

   typedef eosio::multi_index<"powermarket"_n, market> market_index;


    struct [[eosio::table, eosio::contract("unicore")]] exchange_state {
      asset    supply;

      struct connector {
         asset balance;
         double weight = 1;

         EOSLIB_SERIALIZE( connector, (balance)(weight) )
      };

      connector base;
      connector quote;

      uint64_t primary_key()const { return supply.symbol.raw(); }

      asset convert_to_exchange( connector& c, asset in );
      asset convert_from_exchange( connector& c, asset in );
      asset convert( asset from, const symbol& to );

      EOSLIB_SERIALIZE( exchange_state, (supply)(base)(quote) )
   };

   typedef eosio::multi_index< "rammarket"_n, exchange_state > rammarket;

 /// namespace eosiosystem
