# Node++

This is going to be [Silgy](https://github.com/silgy/silgy)'s successor.

There are a couple of breaches that I think need to be introduced in order to ensure the quality and ease of use of the code. I'll keep adding the notes here as I go.

1. OpenSSL linkage will be mandatory. All low-level security functions will be removed from npp_lib.
1. USERS will be based on the [ORM](https://silgy.org/mysqldaogen) layer, thus improving speed and safety. There will be easier to integrate new generated ORM classes into NPP projects.
