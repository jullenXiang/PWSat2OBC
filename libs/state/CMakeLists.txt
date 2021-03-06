set(NAME state)

set(SOURCES
    Include/state/antenna/AntennaConfiguration.hpp
    Include/state/antenna/AntennaState.hpp
    Include/state/time/TimeState.hpp
    Include/state/time/TimeCorrectionConfiguration.hpp
    Include/state/struct.h
    Include/state/PersistentState.hpp
    Include/state/fwd.hpp
    state.cpp
    Antenna.cpp
    time.cpp
    sail.cpp
    ErrorCountersState.cpp
    comm.cpp
    adcs.cpp
)

add_library(${NAME} STATIC ${SOURCES})

target_link_libraries(${NAME}
    base
    time
    adcs
    gsl
    experiments
    error_counter
)

target_include_directories(${NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include/state)

target_format_sources(${NAME} "${SOURCES}")