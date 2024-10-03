set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )
idf_component_register(SRCS "base64.c" "main.c" 
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES  "certs/servercert.pem"
                                    "certs/prvtkey.pem"
                                    "certs/certificate.pem")
#register_component()