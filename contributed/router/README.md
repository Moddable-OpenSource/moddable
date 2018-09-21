# Express Style Router

This is an extremely limited router.

1. It will handle simple get requests
2. It will handle simple post requests
3. Request data will be parsed as JSON if content type is set
4. Response data will be converted to a JSON string

To name but a few of the limitations

1. Will not handle concurrent requests
2. Does not handle chinking of request data
3. Does not do anything aside from above