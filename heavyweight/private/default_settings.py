import secrets

SECRET_KEY = secrets.token_bytes(16)
WTF_CSRF_SECRET_KEY = secrets.token_bytes(16)

LDAP_HOST = "localhost"
LDAP_BASE_DN = "dc=tasteless,dc=eu"
LDAP_USER_DN = "ou=people"
LDAP_SEARCH_FOR_GROUPS = False
LDAP_USER_RDN_ATTR = "uid"
LDAP_USER_LOGIN_ATTR = "uid"
LDAP_USER_OBJECT_FILTER = "(objectclass=account)"
LDAP_BIND_USER_DN = "cn=admin," + LDAP_BASE_DN
LDAP_BIND_USER_PASSWORD = "JonSn0w"

CELERY_BROKER_URL = "redis://localhost:6379"
CELERY_RESULT_BACKEND = "redis://localhost:6379"

CACHE_TYPE = "redis"
CACHE_REDIS_HOST = "localhost"

PUBLIC_URL = "hitme.tasteless.eu"

RESET_TOKEN_LENGTH = 32

MAIL_SENDER_ADDRESS = "noreply@tasteless.eu"