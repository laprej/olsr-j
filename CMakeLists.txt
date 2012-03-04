INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(olsr_srcs
	olsr-driver.c
	olsr-main.c
	olsr.h
)

SET(test_srcs
	olsr-driver.c
	olsr-test.cpp
	olsr.h
)

ADD_EXECUTABLE(olsr-j ${olsr_srcs})

ADD_EXECUTABLE(test-olsr ${test_srcs})

TARGET_LINK_LIBRARIES(olsr-j ROSS m)

TARGET_LINK_LIBRARIES(test-olsr ROSS m)
