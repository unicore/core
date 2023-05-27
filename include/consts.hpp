
#define QUARKS_IN_QUANTS 1000000
#define ONE_PERCENT 10000
#define HUNDR_PERCENT 1000000
#define SYS_PERCENT 100000
#define TOTAL_SEGMENTS 1000000000
#define MAX_CORE_HISTORY_LENGTH 1000

static constexpr eosio::symbol _SYM     = eosio::symbol(eosio::symbol_code("FLOWER"), 4);
static constexpr eosio::symbol _POWER     = eosio::symbol(eosio::symbol_code("POWER"), 0);
static constexpr eosio::symbol _USDT     = eosio::symbol(eosio::symbol_code("USDT"), 4);

static constexpr uint64_t _DATA_ORDER_EXPIRATION = 86400;
static constexpr uint64_t _SHARES_VESTING_DURATION = 604800;
static constexpr uint64_t _WINDOW_SECS = 86400 * 30;//60;// 
static constexpr uint64_t _TOTAL_VOTES = 12;
static constexpr uint64_t _MAX_LEVELS = 26;
static constexpr uint64_t _VOTING_TIME_EXPIRATION = 5; //86400
static constexpr uint64_t _START_VOTES = 1;

static constexpr eosio::name _me = "unicore"_n;
static constexpr eosio::name _partners = "part"_n;
static constexpr eosio::name _curator = "curator"_n;
static constexpr eosio::name _registrator = "registrator"_n;
static constexpr eosio::name _gateway = "gateway"_n;
static constexpr eosio::name _saving = "eosio.saving"_n;
static constexpr eosio::name _core_host = "core"_n;
static constexpr eosio::name _p2p = "p2p"_n;

constexpr uint64_t DEPOSIT = 100;
constexpr uint64_t DEPOSIT_FOR_SOMEONE = 101;
constexpr uint64_t PAY_FOR_HOST = 110;
constexpr uint64_t BUY_BALANCE = 150;
constexpr uint64_t BUY_SHARES = 200;
constexpr uint64_t DONATE_TO_GOAL = 300;
constexpr uint64_t FUND_EMISSION_POOL = 400;

constexpr uint64_t BURN_QUANTS = 800;
constexpr uint64_t SUBSCRIBE_AS_ADVISER = 801;
constexpr uint64_t SUBSCRIBE_AS_ASSISTANT = 802;


constexpr uint64_t SPREAD_TO_REFS = 111;
constexpr uint64_t SPREAD_TO_FUNDS = 112;
constexpr uint64_t SPREAD_TO_DACS = 222;
constexpr uint64_t MAKE_FREE_VESTING = 699;

constexpr uint64_t ADD_INTERNAL_BALANCE = 700;

