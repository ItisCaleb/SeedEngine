if(BUILD_XR)
    target_sources(seed_xr PRIVATE
        xr_engine.cc
    )
endif()