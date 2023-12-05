#include "securite_client.h"



 // fonction pour encrypter les messages :
int encrypte_client(unsigned char *plaintext) {
  //on charge la clé
  EVP_PKEY *pkey = NULL;
  FILE *key_file =fopen("cle_public_serveur.key","rb");
  if(key_file==NULL){
    printf("Le fichier n'a pas pu être chargé");
    exit(0);
  }
  BIO *key_bio = BIO_new_fp(key_file,BIO_CLOSE);
  if(key_bio==NULL){
    printf("Erreur de création de l'objet");
    exit(0);
  }
  pkey=PEM_read_bio_PUBKEY(key_bio,NULL,NULL,NULL);
  
  BIO_free(key_bio);
  fclose(key_file);
  //la clé a été chargée
 
  EVP_PKEY_CTX *ctx;
  ENGINE *eng;
  unsigned char *out;
  size_t outlen, inlen;
  inlen=strlen(plaintext);
  

  /*
  * NB: assumes key, eng, in, inlen are already set up
  * and that key is an RSA private key
  */
  ctx = EVP_PKEY_CTX_new(pkey, eng);
  if (!ctx){
    printf("erreur de création du ctx \n");
    return 0;}
    /* Error occurred */
  if (EVP_PKEY_encrypt_init(ctx) <= 0){return 0;}
    /* Error */
  if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0){return 0;}
    /* Error */

  /* Determine buffer length */
  if (EVP_PKEY_encrypt(ctx, NULL, &outlen,plaintext , inlen) <= 0){return 0;}
    /* Error */

  out = OPENSSL_malloc(outlen);

  if (!out){return 0;}
    /* malloc failure */

  if (EVP_PKEY_encrypt(ctx, out, &outlen, plaintext, inlen) <= 0){return 0;}
    /* Error */
  
  //recopier le message chiffré dans le buffer. Pour cela faire un realloc pour augmenter l'espace du buffer (ou donner un espace assez large de base) puis strcopy

  realloc(plaintext,outlen);
  strcpy(plaintext,out);

  return 1;
  

  
  

}





//fonction pour decrypter les messages
int decrypte_client(unsigned char *ciphertext) {
  //chargement de la clé
  //on charge la clé
  EVP_PKEY *pkey = NULL;
  FILE *key_file =fopen("cle_prive_voiture.key","rb");
  if(key_file==NULL){
    printf("Le fichier n'a pas pu être chargé");
    exit(0);
  }
  BIO *key_bio = BIO_new_fp(key_file,BIO_CLOSE);
  if(key_bio==NULL){
    printf("Erreur de création de l'objet");
    exit(0);
  }
  pkey=PEM_read_bio_PrivateKey(key_bio,NULL,NULL,NULL);
  
  BIO_free(key_bio);
  fclose(key_file);
  //decryptage

  EVP_PKEY_CTX *ctx;
  ENGINE *eng;
  unsigned char *out;
  size_t outlen, inlen;
  inlen=strlen(ciphertext);
  

  /*
  * NB: assumes key, eng, in, inlen are already set up
  * and that key is an RSA private key
  */
  ctx = EVP_PKEY_CTX_new(pkey, eng);
  if (!ctx){
    printf("erreur de création du ctx \n");
    return 0;}
    /* Error occurred */
  if (EVP_PKEY_decrypt_init(ctx) <= 0){return 0;}
    /* Error */
  if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0){return 0;}
    /* Error */

  /* Determine buffer length */
  if (EVP_PKEY_decrypt(ctx, NULL, &outlen,ciphertext , inlen) <= 0){return 0;}
    /* Error */

  out = OPENSSL_malloc(outlen);

  if (!out){return 0;}
    /* malloc failure */

  if (EVP_PKEY_decrypt(ctx, out, &outlen, ciphertext, inlen) <= 0){return 0;}
    /* Error */
  
  //recopier le message chiffré dans le buffer. Pour cela faire un realloc pour augmenter l'espace du buffer (ou donner un espace assez large de base) puis strcopy

  realloc(ciphertext,outlen);
  strcpy(ciphertext,out);

  //libérer l'espace utilisé qui n'est plus nécessaire
  return 1;
  

}







  
