// See https://github.com/philsquared/Catch/wiki/Tutorial for quick info.

#define CATCH_CONFIG_MAIN  // This tell CATCH to provide a main()
                           // only do this in one cpp file
#include "catch.hpp"

// Make sure all the types, variables, functions, etc. that you need
// are available.

typedef uintptr_t o_addr;
int world_size;
unsigned int SA_range_start;

// Catch is a C++ testing framework.  As such, you need to tell it to look
// for things the way they are specified in C, i.e. function names should
// not be mangled.
extern "C" {
    o_addr sa_master_for_level(o_addr lpid);
}

// A really simple test case
TEST_CASE("foo", "A simple test case")
{
    REQUIRE( 0 == 0 );
}

// A more advance test case, but still quite simple.  Here we are just
// exercising our sa_master_for_level() function using "basic"
// parameters.  We are being overly pedantic here.
TEST_CASE("sa_master_for_level/simple", "Testing SMFL function")
{
    // You must ensure that all variables used in your functions are
    // set up correctly prior to calling them!!!  Note that here we
    // must also set world_size as that is returned via tw_nnodes().
    world_size = 1;
    SA_range_start = 16;
    
    REQUIRE ( 16 == sa_master_for_level(0) );
    REQUIRE ( 16 == sa_master_for_level(1) );
    REQUIRE ( 16 == sa_master_for_level(2) );
    REQUIRE ( 16 == sa_master_for_level(3) );
    REQUIRE ( 16 == sa_master_for_level(4) );
    REQUIRE ( 16 == sa_master_for_level(5) );
    REQUIRE ( 16 == sa_master_for_level(6) );
    REQUIRE ( 16 == sa_master_for_level(7) );
    REQUIRE ( 16 == sa_master_for_level(8) );
    REQUIRE ( 16 == sa_master_for_level(9) );
    REQUIRE ( 16 == sa_master_for_level(10) );
    REQUIRE ( 16 == sa_master_for_level(11) );
    REQUIRE ( 16 == sa_master_for_level(12) );
    REQUIRE ( 16 == sa_master_for_level(13) );
    REQUIRE ( 16 == sa_master_for_level(14) );
    REQUIRE ( 16 == sa_master_for_level(15) );
    // This one should fail.
    REQUIRE ( 16 == sa_master_for_level(16) );
}

TEST_CASE("master_hierarchy/simple", "Testing the MA function")
{
    
}