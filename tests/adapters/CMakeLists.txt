
# Check if the condition to enable state updater adapters is met
if(ENABLE_STATE_UPDATER_ADAPTERS)
    # Include the state_updater subdirectory for further processing# Include the state_updater subdirectory in the build
    add_subdirectory(state_updater)
endif()
