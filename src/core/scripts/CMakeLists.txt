install(
  FILES
    gino
    gino-autotuner
    gino-enable
    gino-inline
    gino-io
    gino-load
    gino-loops
    gino-planner
    gino-pre
  DESTINATION
    bin
  PERMISSIONS
    OWNER_EXECUTE OWNER_WRITE OWNER_READ
    GROUP_EXECUTE GROUP_READ
    WORLD_EXECUTE WORLD_READ
)
