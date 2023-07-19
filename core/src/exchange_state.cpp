
using namespace eosio;

    asset market::convert_to_exchange( connector& reserve, const asset& payment )
   {
      const double S0 = supply.amount;
      const double R0 = reserve.balance.amount;
      const double dR = payment.amount;
      const double F  = reserve.weight;

      double dS = S0 * ( std::pow(1. + dR / R0, F) - 1. );
      if ( dS < 0 ) dS = 0; // rounding errors
      reserve.balance += payment;
      supply.amount   += int64_t(dS);
      return asset( int64_t(dS), supply.symbol );
   }

   asset market::convert_from_exchange( connector& reserve, const asset& tokens )
   {
      const double R0 = reserve.balance.amount;
      const double S0 = supply.amount;
      const double dS = -tokens.amount; // dS < 0, tokens are subtracted from supply
      const double Fi = double(1) / reserve.weight;

      double dR = R0 * ( std::pow(1. + dS / S0, Fi) - 1. ); // dR < 0 since dS < 0
      if ( dR > 0 ) dR = 0; // rounding errors
      reserve.balance.amount -= int64_t(-dR);
      supply                 -= tokens;
      return asset( int64_t(-dR), reserve.balance.symbol );
   }

   asset market::convert( const asset& from, const symbol& to )
   {
      const auto& sell_symbol  = from.symbol;
      const auto& base_symbol  = base.balance.symbol;
      const auto& quote_symbol = quote.balance.symbol;
      eosio::check( sell_symbol != to, "cannot convert to the same symbol" );

      asset out( 0, to );
      if ( sell_symbol == base_symbol && to == quote_symbol ) {
         const asset tmp = convert_to_exchange( base, from );
         out = convert_from_exchange( quote, tmp );
      } else if ( sell_symbol == quote_symbol && to == base_symbol ) {
         const asset tmp = convert_to_exchange( quote, from );
         out = convert_from_exchange( base, tmp );
      } else {
         eosio::check( false, "invalid conversion" );
      }
      return out;
   }

   asset market::direct_convert( const asset& from, const symbol& to )
   {
      const auto& sell_symbol  = from.symbol;
      const auto& base_symbol  = base.balance.symbol;
      const auto& quote_symbol = quote.balance.symbol;
      eosio::check( sell_symbol != to, "cannot convert to the same symbol" );

      asset out( 0, to );
      if ( sell_symbol == base_symbol && to == quote_symbol ) {
         out.amount = get_bancor_output( base.balance.amount, quote.balance.amount, from.amount );
         base.balance  += from;
         quote.balance -= out;
      } else if ( sell_symbol == quote_symbol && to == base_symbol ) {
         out.amount = get_bancor_output( quote.balance.amount, base.balance.amount, from.amount );
         quote.balance += from;
         base.balance  -= out;
      } else {
         eosio::check( false, "invalid conversion" );
      }
      return out;
   }

   int64_t market::get_bancor_output( int64_t inp_reserve,
                                              int64_t out_reserve,
                                              int64_t inp )
   {
      const double ib = inp_reserve;
      const double ob = out_reserve;
      const double in = inp;
      
      int64_t out = int64_t( (in * ob) / (ib + in) );
      
      if ( out < 0 ) out = 0;

      return out;
   }

   int64_t market::get_bancor_input( int64_t out_reserve,
                                             int64_t inp_reserve,
                                             int64_t out )
   {
      const double ob = out_reserve;
      const double ib = inp_reserve;

      int64_t inp = (ib * out) / (ob - out);

      if ( inp < 0 ) inp = 0;

      return inp;
   }

   asset exchange_state::convert_to_exchange( connector& c, asset in ) {

      real_type R(supply.amount);
      real_type C(c.balance.amount+in.amount);
      real_type F(c.weight);
      real_type T(in.amount);
      real_type ONE(1.0);

      real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
      int64_t issued = int64_t(E);

      supply.amount += issued;
      c.balance.amount += in.amount;

      return asset( issued, supply.symbol );
   }

   asset exchange_state::convert_from_exchange( connector& c, asset in ) {
      check( in.symbol== supply.symbol, "unexpected asset symbol input" );

      real_type R(supply.amount - in.amount);
      real_type C(c.balance.amount);
      real_type F(1.0/c.weight);
      real_type E(in.amount);
      real_type ONE(1.0);


     // potentially more accurate: 
     // The functions std::expm1 and std::log1p are useful for financial calculations, for example, 
     // when calculating small daily interest rates: (1+x)n
     // -1 can be expressed as std::expm1(n * std::log1p(x)). 
     // real_type T = C * std::expm1( F * std::log1p(E/R) );
      
      real_type T = C * (std::pow( ONE + E/R, F) - ONE);
      int64_t out = int64_t(T);

      supply.amount -= in.amount;
      c.balance.amount -= out;

      return asset( out, c.balance.symbol );
   }

   asset exchange_state::convert( asset from, const symbol& to ) {
      auto sell_symbol  = from.symbol;
      auto ex_symbol    = supply.symbol;
      auto base_symbol  = base.balance.symbol;
      auto quote_symbol = quote.balance.symbol;

      //print( "From: ", from, " TO ", asset( 0,to), "\n" );
      //print( "base: ", base_symbol, "\n" );
      //print( "quote: ", quote_symbol, "\n" );
      //print( "ex: ", supply.symbol, "\n" );

      if( sell_symbol != ex_symbol ) {
         if( sell_symbol == base_symbol ) {
            from = convert_to_exchange( base, from );
         } else if( sell_symbol == quote_symbol ) {
            from = convert_to_exchange( quote, from );
         } else { 
            check( false, "invalid sell" );
         }
      } else {
         if( to == base_symbol ) {
            from = convert_from_exchange( base, from ); 
         } else if( to == quote_symbol ) {
            from = convert_from_exchange( quote, from ); 
         } else {
            check( false, "invalid conversion" );
         }
      }

      if( to != from.symbol )
         return convert( from, to );

      return from;
   }


 /// namespace eosiosystem
