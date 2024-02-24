install(
  FILES
    gino-loop-single
    gino-loop-subset
    gino-plan-info
    gino-time-saved
  DESTINATION
    bin
  PERMISSIONS
    OWNER_EXECUTE OWNER_WRITE OWNER_READ
    GROUP_EXECUTE GROUP_READ
    WORLD_EXECUTE WORLD_READ
)
