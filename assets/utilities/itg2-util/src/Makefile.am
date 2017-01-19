bin_PROGRAMS = itg2ac-util

KeydumpSources = keydump.c dbgutil.c config.h keydump.h

iButtonSources = crcutil.c ds2480ut.c linuxlnk.c owerr.c owllu.c ownetu.c \
owsesu.c owtrnu.c sha18.c shaib.c getkey.c ds2480.h owfile.h ownet.h shaib.h getkey.h

AESSources = aescrypt.c aeskey.c aestab.c aes.h aesopt.h aestab.h

#patch_SOURCES = $(KeydumpSources) $(AESSources)
data_SOURCES = $(KeydumpSources) $(iButtonSources) $(AESSources)

#################################################################

###### ITG2 ############

LIBS = -lgcrypt -lgpg-error

itg2ac_util_SOURCES = $(data_SOURCES) itg2-swiss-army-knife.c itg2util.c itg2util.h itg2-patch-constants.h
itg2ac_util_LDADD = 
## libtommath ##

libtommath_a_SOURCES = \
libtommath/bncore.c libtommath/bn_mp_init.c libtommath/bn_mp_clear.c libtommath/bn_mp_exch.c libtommath/bn_mp_grow.c libtommath/bn_mp_shrink.c \
libtommath/bn_mp_clamp.c libtommath/bn_mp_zero.c  libtommath/bn_mp_set.c libtommath/bn_mp_set_int.c libtommath/bn_mp_init_size.c libtommath/bn_mp_copy.c \
libtommath/bn_mp_init_copy.c libtommath/bn_mp_abs.c libtommath/bn_mp_neg.c libtommath/bn_mp_cmp_mag.c libtommath/bn_mp_cmp.c libtommath/bn_mp_cmp_d.c \
libtommath/bn_mp_rshd.c libtommath/bn_mp_lshd.c libtommath/bn_mp_mod_2d.c libtommath/bn_mp_div_2d.c libtommath/bn_mp_mul_2d.c libtommath/bn_mp_div_2.c \
libtommath/bn_mp_mul_2.c libtommath/bn_s_mp_add.c libtommath/bn_s_mp_sub.c libtommath/bn_fast_s_mp_mul_digs.c libtommath/bn_s_mp_mul_digs.c \
libtommath/bn_fast_s_mp_mul_high_digs.c libtommath/bn_s_mp_mul_high_digs.c libtommath/bn_fast_s_mp_sqr.c libtommath/bn_s_mp_sqr.c \
libtommath/bn_mp_add.c libtommath/bn_mp_sub.c libtommath/bn_mp_karatsuba_mul.c libtommath/bn_mp_mul.c libtommath/bn_mp_karatsuba_sqr.c \
libtommath/bn_mp_sqr.c libtommath/bn_mp_div.c libtommath/bn_mp_mod.c libtommath/bn_mp_add_d.c libtommath/bn_mp_sub_d.c libtommath/bn_mp_mul_d.c \
libtommath/bn_mp_div_d.c libtommath/bn_mp_mod_d.c libtommath/bn_mp_expt_d.c libtommath/bn_mp_addmod.c libtommath/bn_mp_submod.c \
libtommath/bn_mp_mulmod.c libtommath/bn_mp_sqrmod.c libtommath/bn_mp_gcd.c libtommath/bn_mp_lcm.c libtommath/bn_fast_mp_invmod.c libtommath/bn_mp_invmod.c \
libtommath/bn_mp_reduce.c libtommath/bn_mp_montgomery_setup.c libtommath/bn_fast_mp_montgomery_reduce.c libtommath/bn_mp_montgomery_reduce.c \
libtommath/bn_mp_exptmod_fast.c libtommath/bn_mp_exptmod.c libtommath/bn_mp_2expt.c libtommath/bn_mp_n_root.c libtommath/bn_mp_jacobi.c libtommath/bn_reverse.c \
libtommath/bn_mp_count_bits.c libtommath/bn_mp_read_unsigned_bin.c libtommath/bn_mp_read_signed_bin.c libtommath/bn_mp_to_unsigned_bin.c \
libtommath/bn_mp_to_signed_bin.c libtommath/bn_mp_unsigned_bin_size.c libtommath/bn_mp_signed_bin_size.c  \
libtommath/bn_mp_xor.c libtommath/bn_mp_and.c libtommath/bn_mp_or.c libtommath/bn_mp_rand.c libtommath/bn_mp_montgomery_calc_normalization.c \
libtommath/bn_mp_prime_is_divisible.c libtommath/bn_prime_tab.c libtommath/bn_mp_prime_fermat.c libtommath/bn_mp_prime_miller_rabin.c \
libtommath/bn_mp_prime_is_prime.c libtommath/bn_mp_prime_next_prime.c libtommath/bn_mp_dr_reduce.c \
libtommath/bn_mp_dr_is_modulus.c libtommath/bn_mp_dr_setup.c libtommath/bn_mp_reduce_setup.c \
libtommath/bn_mp_toom_mul.c libtommath/bn_mp_toom_sqr.c libtommath/bn_mp_div_3.c libtommath/bn_s_mp_exptmod.c \
libtommath/bn_mp_reduce_2k.c libtommath/bn_mp_reduce_is_2k.c libtommath/bn_mp_reduce_2k_setup.c \
libtommath/bn_mp_reduce_2k_l.c libtommath/bn_mp_reduce_is_2k_l.c libtommath/bn_mp_reduce_2k_setup_l.c \
libtommath/bn_mp_radix_smap.c libtommath/bn_mp_read_radix.c libtommath/bn_mp_toradix.c libtommath/bn_mp_radix_size.c \
libtommath/bn_mp_fread.c libtommath/bn_mp_fwrite.c libtommath/bn_mp_cnt_lsb.c libtommath/bn_error.c \
libtommath/bn_mp_init_multi.c libtommath/bn_mp_clear_multi.c libtommath/bn_mp_exteuclid.c libtommath/bn_mp_toradix_n.c \
libtommath/bn_mp_prime_random_ex.c libtommath/bn_mp_get_int.c libtommath/bn_mp_sqrt.c libtommath/bn_mp_is_square.c libtommath/bn_mp_init_set.c \
libtommath/bn_mp_init_set_int.c libtommath/bn_mp_invmod_slow.c libtommath/bn_mp_prime_rabin_miller_trials.c \
libtommath/bn_mp_to_signed_bin_n.c libtommath/bn_mp_to_unsigned_bin_n.c \
libtommath/tommath_class.h libtommath/tommath.h libtommath/tommath_superclass.h
EXTRA_DIST = libtommath/bn_prime_tab.c
noinst_LIBRARIES = libtommath.a
libtommath_a_CFLAGS = -I$(srcdir)/libtommath $(AM_CFLAGS)

## libtomcrypt ##

libtomcrypt_a_SOURCES = \
libtomcrypt/src/ciphers/aes/aes.c \
libtomcrypt/src/hashes/md5.c libtomcrypt/src/hashes/sha1.c libtomcrypt/src/hashes/helper/hash_memory.c \
libtomcrypt/src/math/fp/ltc_ecc_fp_mulmod.c libtomcrypt/src/math/ltm_desc.c libtomcrypt/src/math/multi.c \
libtomcrypt/src/math/rand_prime.c libtomcrypt/src/misc/base64/base64_decode.c \
libtomcrypt/src/misc/base64/base64_encode.c libtomcrypt/src/misc/burn_stack.c libtomcrypt/src/misc/crypt/crypt.c \
libtomcrypt/src/misc/crypt/crypt_argchk.c libtomcrypt/src/misc/crypt/crypt_cipher_descriptor.c \
libtomcrypt/src/misc/crypt/crypt_cipher_is_valid.c libtomcrypt/src/misc/crypt/crypt_find_cipher.c \
libtomcrypt/src/misc/crypt/crypt_find_cipher_any.c libtomcrypt/src/misc/crypt/crypt_find_cipher_id.c \
libtomcrypt/src/misc/crypt/crypt_find_hash.c libtomcrypt/src/misc/crypt/crypt_find_hash_any.c \
libtomcrypt/src/misc/crypt/crypt_find_hash_id.c libtomcrypt/src/misc/crypt/crypt_find_hash_oid.c \
libtomcrypt/src/misc/crypt/crypt_find_prng.c libtomcrypt/src/misc/crypt/crypt_fsa.c libtomcrypt/src/misc/crypt/crypt_hash_descriptor.c \
libtomcrypt/src/misc/crypt/crypt_hash_is_valid.c libtomcrypt/src/misc/crypt/crypt_ltc_mp_descriptor.c \
libtomcrypt/src/misc/crypt/crypt_prng_descriptor.c libtomcrypt/src/misc/crypt/crypt_prng_is_valid.c \
libtomcrypt/src/misc/crypt/crypt_register_cipher.c libtomcrypt/src/misc/crypt/crypt_register_hash.c \
libtomcrypt/src/misc/crypt/crypt_register_prng.c libtomcrypt/src/misc/crypt/crypt_unregister_cipher.c \
libtomcrypt/src/misc/crypt/crypt_unregister_hash.c libtomcrypt/src/misc/crypt/crypt_unregister_prng.c \
libtomcrypt/src/misc/error_to_string.c libtomcrypt/src/misc/pkcs5/pkcs_5_1.c libtomcrypt/src/misc/pkcs5/pkcs_5_2.c libtomcrypt/src/misc/zeromem.c \
libtomcrypt/src/modes/cbc/cbc_decrypt.c libtomcrypt/src/modes/cbc/cbc_done.c libtomcrypt/src/modes/cbc/cbc_encrypt.c \
libtomcrypt/src/modes/cbc/cbc_getiv.c libtomcrypt/src/modes/cbc/cbc_setiv.c libtomcrypt/src/modes/cbc/cbc_start.c \
libtomcrypt/src/modes/cfb/cfb_decrypt.c libtomcrypt/src/modes/cfb/cfb_done.c libtomcrypt/src/modes/cfb/cfb_encrypt.c \
libtomcrypt/src/modes/cfb/cfb_getiv.c libtomcrypt/src/modes/cfb/cfb_setiv.c libtomcrypt/src/modes/cfb/cfb_start.c \
libtomcrypt/src/modes/ctr/ctr_decrypt.c libtomcrypt/src/modes/ctr/ctr_done.c libtomcrypt/src/modes/ctr/ctr_encrypt.c \
libtomcrypt/src/modes/ctr/ctr_getiv.c libtomcrypt/src/modes/ctr/ctr_setiv.c libtomcrypt/src/modes/ctr/ctr_start.c libtomcrypt/src/modes/ctr/ctr_test.c \
libtomcrypt/src/modes/ecb/ecb_decrypt.c libtomcrypt/src/modes/ecb/ecb_done.c libtomcrypt/src/modes/ecb/ecb_encrypt.c \
libtomcrypt/src/modes/ecb/ecb_start.c \
libtomcrypt/src/modes/ofb/ofb_decrypt.c libtomcrypt/src/modes/ofb/ofb_done.c \
libtomcrypt/src/pk/asn1/der/bit/der_decode_bit_string.c \
libtomcrypt/src/pk/asn1/der/bit/der_encode_bit_string.c libtomcrypt/src/pk/asn1/der/bit/der_length_bit_string.c \
libtomcrypt/src/pk/asn1/der/boolean/der_decode_boolean.c libtomcrypt/src/pk/asn1/der/boolean/der_encode_boolean.c \
libtomcrypt/src/pk/asn1/der/boolean/der_length_boolean.c libtomcrypt/src/pk/asn1/der/choice/der_decode_choice.c \
libtomcrypt/src/pk/asn1/der/ia5/der_decode_ia5_string.c libtomcrypt/src/pk/asn1/der/ia5/der_encode_ia5_string.c \
libtomcrypt/src/pk/asn1/der/ia5/der_length_ia5_string.c libtomcrypt/src/pk/asn1/der/integer/der_decode_integer.c \
libtomcrypt/src/pk/asn1/der/integer/der_encode_integer.c libtomcrypt/src/pk/asn1/der/integer/der_length_integer.c \
libtomcrypt/src/pk/asn1/der/object_identifier/der_decode_object_identifier.c \
libtomcrypt/src/pk/asn1/der/object_identifier/der_encode_object_identifier.c \
libtomcrypt/src/pk/asn1/der/object_identifier/der_length_object_identifier.c \
libtomcrypt/src/pk/asn1/der/octet/der_decode_octet_string.c libtomcrypt/src/pk/asn1/der/octet/der_encode_octet_string.c \
libtomcrypt/src/pk/asn1/der/octet/der_length_octet_string.c \
libtomcrypt/src/pk/asn1/der/printable_string/der_decode_printable_string.c \
libtomcrypt/src/pk/asn1/der/printable_string/der_encode_printable_string.c \
libtomcrypt/src/pk/asn1/der/printable_string/der_length_printable_string.c \
libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_ex.c \
libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_flexi.c \
libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_multi.c \
libtomcrypt/src/pk/asn1/der/sequence/der_encode_sequence_ex.c \
libtomcrypt/src/pk/asn1/der/sequence/der_encode_sequence_multi.c libtomcrypt/src/pk/asn1/der/sequence/der_length_sequence.c \
libtomcrypt/src/pk/asn1/der/sequence/der_sequence_free.c libtomcrypt/src/pk/asn1/der/set/der_encode_set.c \
libtomcrypt/src/pk/asn1/der/set/der_encode_setof.c libtomcrypt/src/pk/asn1/der/short_integer/der_decode_short_integer.c \
libtomcrypt/src/pk/asn1/der/short_integer/der_encode_short_integer.c \
libtomcrypt/src/pk/asn1/der/short_integer/der_length_short_integer.c libtomcrypt/src/pk/asn1/der/utctime/der_decode_utctime.c \
libtomcrypt/src/pk/asn1/der/utctime/der_encode_utctime.c libtomcrypt/src/pk/asn1/der/utctime/der_length_utctime.c \
libtomcrypt/src/pk/asn1/der/utf8/der_decode_utf8_string.c libtomcrypt/src/pk/asn1/der/utf8/der_encode_utf8_string.c \
libtomcrypt/src/pk/asn1/der/utf8/der_length_utf8_string.c libtomcrypt/src/pk/dsa/dsa_decrypt_key.c \
libtomcrypt/src/pk/dsa/dsa_encrypt_key.c libtomcrypt/src/pk/dsa/dsa_export.c libtomcrypt/src/pk/dsa/dsa_free.c libtomcrypt/src/pk/dsa/dsa_import.c \
libtomcrypt/src/pk/dsa/dsa_make_key.c libtomcrypt/src/pk/dsa/dsa_shared_secret.c libtomcrypt/src/pk/dsa/dsa_sign_hash.c \
libtomcrypt/src/pk/dsa/dsa_verify_hash.c libtomcrypt/src/pk/dsa/dsa_verify_key.c \
libtomcrypt/src/pk/pkcs1/pkcs_1_i2osp.c libtomcrypt/src/pk/pkcs1/pkcs_1_mgf1.c \
libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_decode.c libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_encode.c libtomcrypt/src/pk/pkcs1/pkcs_1_os2ip.c \
libtomcrypt/src/pk/pkcs1/pkcs_1_pss_decode.c libtomcrypt/src/pk/pkcs1/pkcs_1_pss_encode.c libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_decode.c \
libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_encode.c libtomcrypt/src/pk/rsa/rsa_decrypt_key.c libtomcrypt/src/pk/rsa/rsa_encrypt_key.c \
libtomcrypt/src/pk/rsa/rsa_export.c libtomcrypt/src/pk/rsa/rsa_exptmod.c libtomcrypt/src/pk/rsa/rsa_free.c libtomcrypt/src/pk/rsa/rsa_import.c \
libtomcrypt/src/pk/rsa/rsa_make_key.c libtomcrypt/src/pk/rsa/rsa_sign_hash.c libtomcrypt/src/pk/rsa/rsa_verify_hash.c libtomcrypt/src/prngs/fortuna.c \
libtomcrypt/src/prngs/rng_get_bytes.c libtomcrypt/src/prngs/rng_make_prng.c \
libtomcrypt/src/prngs/sprng.c libtomcrypt/src/prngs/yarrow.c \
libtomcrypt/src/headers/tomcrypt_argchk.h libtomcrypt/src/headers/tomcrypt_cfg.h libtomcrypt/src/headers/tomcrypt_cipher.h \
libtomcrypt/src/headers/tomcrypt_custom.h libtomcrypt/src/headers/tomcrypt.h libtomcrypt/src/headers/tomcrypt_hash.h \
libtomcrypt/src/headers/tomcrypt_mac.h libtomcrypt/src/headers/tomcrypt_macros.h libtomcrypt/src/headers/tomcrypt_math.h \
libtomcrypt/src/headers/tomcrypt_misc.h libtomcrypt/src/headers/tomcrypt_pkcs.h libtomcrypt/src/headers/tomcrypt_pk.h \
libtomcrypt/src/headers/tomcrypt_prng.h
EXTRA_DIST += libtomcrypt/src/ciphers/aes/aes_tab.c libtomcrypt/src/prngs/sober128tab.c
noinst_LIBRARIES += libtomcrypt.a
libtomcrypt_a_CFLAGS = -I$(srcdir)/libtomcrypt/src/headers $(AM_CFLAGS)
