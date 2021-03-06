/*******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 ******************************************************************************/


//==========================================================
// Includes
//

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include <aerospike/aerospike.h>
#include <aerospike/aerospike_batch.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/as_batch.h>
#include <aerospike/as_error.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_key.h>
#include <aerospike/as_record.h>
#include <aerospike/as_status.h>
#include <aerospike/as_scan.h>

#include "example_utils.h"

const as_namespace k_namespace = "kdemo";
const as_set k_set = "kset";

//==========================================================
// Forward Declarations
//

bool batch_read_cb(const as_batch_read* results, uint32_t n, void* udata);
void cleanup(aerospike* p_as);
bool insert_records(aerospike* p_as);


void
remove_test_records(aerospike* p_as)
{
	// Multiple-record examples insert g_n_keys records, using integer keys from
	// 0 to (g_n_keys - 1).
	for (uint32_t i = 0; i < g_n_keys; i++) {
		as_error err;

		// No need to destroy a stack as_key object, if we only use
		// as_key_init_int64().
		as_key key;
		
		uint64_t keyval[2];
		keyval[0] = i;
		keyval[1] = i;
		
		as_key_init_raw(&key, g_namespace, k_set, (uint8_t*)(keyval), 16);    
                
		as_record* p_rec = NULL;

		if (aerospike_key_get(p_as, &err, NULL, &key, &p_rec) != AEROSPIKE_OK) {
    			fprintf(stderr, "err(%d) %s at [%s:%d]\n", err.code, err.message, err.file, err.line);
		}
		uint8_t* keys =  as_bytes_get((as_bytes*) (key.valuep));		
		
		LOG("index %u, key %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X :", i, *keys,keys[1],keys[2],keys[3],keys[4],keys[5],keys[6],keys[7],*(keys+8),keys[9],keys[10],keys[11],keys[12],keys[13],keys[14],keys[15]);
  

		// Ignore errors - just trying to leave the database as we found it.
		if(aerospike_key_remove(p_as, &err, NULL, &key) != AEROSPIKE_OK) {
			LOG("aerospike_key_remove() returned %d - %s", err.code, err.message);
			//cleanup(p_as);
			//exit(-1);
		}
	}

	LOG("All keys are removed!");
}

//==========================================================
// BATCH GET Example
//

int
main(int argc, char* argv[])
{
	// Parse command line arguments.
	if (! example_get_opts(argc, argv, EXAMPLE_MULTI_KEY_OPTS)) {
		exit(-1);
	}

	// Connect to the aerospike database cluster.
	aerospike as;
	example_connect_to_aerospike(&as);
	
/*
	as_config config;
	as_config_init(&config);

	config.policies.write.key = AS_POLICY_KEY_SEND;
	config.policies.read.key = AS_POLICY_KEY_SEND;
	config.policies.key =  AS_POLICY_KEY_SEND;
*/
	// Start clean.
	remove_test_records(&as);

	
	if (! insert_records(&as)) {
		cleanup(&as);
		exit(-1);
	}


	LOG("read the records just inserted.");
        for (uint32_t i = 0; i < g_n_keys; i++) {
		as_error err;

		// No need to destroy a stack as_key object, if we only use
		// as_key_init_int64().
		as_key key;
		
		uint64_t keyval[2];
		keyval[0] = i;
		keyval[1] = i;
		
		as_key_init_raw(&key, g_namespace, k_set, (uint8_t*)(keyval), 16);    
                
		as_record* p_rec = NULL;

		if (aerospike_key_get(&as, &err, NULL, &key, &p_rec) != AEROSPIKE_OK) {
    			fprintf(stderr, "err(%d) %s at [%s:%d]\n", err.code, err.message, err.file, err.line);
		}
		uint8_t* keys =  as_bytes_get((as_bytes*) (key.valuep));		
		
		LOG("index %u, key %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X :", i, *keys,keys[1],keys[2],keys[3],keys[4],keys[5],keys[6],keys[7],*(keys+8),keys[9],keys[10],keys[11],keys[12],keys[13],keys[14],keys[15]);
                example_dump_record(p_rec);		
	}

	LOG("all records just inserted are read.");
	as_error err;

	// Make a batch of all the keys we inserted.
	as_batch batch;
	as_batch_inita(&batch, g_n_keys);
	uint64_t keyval[2 * g_n_keys];
	LOG("set up batch with keys.");
	for (uint32_t i = 0; i < g_n_keys; i++) {
		keyval[i * 2] = i;
		keyval[i * 2 + 1] = i;
		
		as_key_init_raw(as_batch_keyat(&batch, i), g_namespace, k_set, (uint8_t*)(&keyval[i * 2]), 16);      

		uint8_t* keys =  as_bytes_get((as_bytes*) (as_batch_keyat(&batch, i)->valuep));		
		
		LOG("index %u, key %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X :", i, *keys,keys[1],keys[2],keys[3],keys[4],keys[5],keys[6],keys[7],*(keys+8),keys[9],keys[10],keys[11],keys[12],keys[13],keys[14],keys[15]);          
	}
	LOG("set up batch with keys done.");
	/*
	// Check existence of these keys - they should all be there.
	if (aerospike_batch_exists(&as, &err, NULL, &batch, batch_read_cb, NULL) !=
			AEROSPIKE_OK) {
		LOG("aerospike_batch_exists() returned %d - %s", err.code, err.message);
		cleanup(&as);
		exit(-1);
	}

	LOG("batch exists call completed");
	*/
	LOG("check batch keys.");
	for (uint32_t i = 0; i < g_n_keys; i++) {	    

		uint8_t* keys =  as_bytes_get((as_bytes*) ((as_batch_keyat(&batch, i)->valuep)));		
		
		LOG("index %u, key address: %" PRId64 ", key %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X :", i, as_batch_keyat(&batch, i)->valuep,*keys,keys[1],keys[2],keys[3],keys[4],keys[5],keys[6],keys[7],*(keys+8),keys[9],keys[10],keys[11],keys[12],keys[13],keys[14],keys[15]);          
	}
	LOG("check batch keys done.");
	
	// Get all of these keys - they should all be there.
	if (aerospike_batch_get(&as, &err, NULL, &batch, batch_read_cb, NULL) !=
			AEROSPIKE_OK) {
		LOG("aerospike_batch_get() returned %d - %s", err.code, err.message);
		cleanup(&as);
		exit(-1);
	}

	LOG("batch get call completed");

	as_batch_destroy(&batch);

	// Cleanup and disconnect from the database cluster.
	cleanup(&as);

	LOG("kinaxis batch get example successfully completed");

	return 0;
}


//==========================================================
// Batch Callback
//

bool
batch_read_cb(const as_batch_read* results, uint32_t n, void* udata)
{
	LOG("batch read callback returned %u/%u record results:", n, g_n_keys);

	uint32_t n_found = 0;

	for (uint32_t i = 0; i < n; i++) {
		
		//uint8_t* keys =  as_bytes_get((as_bytes*) (results[i].key->valuep));		
		
		//LOG("index %u, key %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X :", i, *keys,keys[1],keys[2],keys[3],keys[4],keys[5],keys[6],keys[7],*(keys+8),keys[9],keys[10],keys[11],keys[12],keys[13],keys[14],keys[15]);
		//LOG("index %u, key %" PRId64 ":", i, keys);
		
		
		if (results[i].result == AEROSPIKE_OK) {
			//LOG("  AEROSPIKE_OK");
			// For aerospike_batch_exists() calls, there should be record
			// metadata but no bins.
			example_dump_record(&results[i].record);
			n_found++;
		}
		else if (results[i].result == AEROSPIKE_ERR_RECORD_NOT_FOUND) {
			// The transaction succeeded but the record doesn't exist.
			LOG("  AEROSPIKE_ERR_RECORD_NOT_FOUND");
		}
		else {
			// The transaction didn't succeed.
			LOG("  error %d", results[i].result);
		}
	}

	LOG("... found %u/%u records", n_found, n);

	return true;
}


//==========================================================
// Helpers
//

void
cleanup(aerospike* p_as)
{
	example_remove_test_records(p_as);
	example_cleanup(p_as);
}

bool
insert_records(aerospike* p_as)
{
	// Create an as_record object with one (integer value) bin. By using
	// as_record_inita(), we won't need to destroy the record if we only set
	// bins using as_record_set_int64().
	

	// Re-using rec, write records into the database such that each record's key
	// and (test-bin) value is based on the loop index.
	for (uint32_t i = 0; i < g_n_keys; i++) {
		as_error err;

		// No need to destroy a stack as_key object, if we only use
		// as_key_init_int64().
		as_key key;
		
		uint64_t keyval[2];
		keyval[0] = i;
		keyval[1] = i;		
		
		as_key_init_raw(&key, g_namespace, k_set, (uint8_t*)(keyval), 16);
		
		uint8_t* key16 = as_bytes_get((as_bytes*) key.valuep);
		LOG("insert a key: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X : ", key16[0], key16[1], key16[2], key16[3], key16[4], key16[5], key16[6], key16[7],key16[8], key16[9], key16[10], key16[11],key16[12], key16[13], key16[14], key16[15]);

		// In general it's ok to reset a bin value - all as_record_set_... calls
		// destroy any previous value.
		as_record rec;
		as_record_inita(&rec, 1);
		as_record_set_int64(&rec, "kbin1", (uint64_t)i);

		// Write a record to the database.
		if (aerospike_key_put(p_as, &err, NULL, &key, &rec) != AEROSPIKE_OK) {
			LOG("aerospike_key_put() returned %d - %s", err.code, err.message);
			return false;
		}

		as_key_destroy(&key);
		as_record_destroy(&rec);
	}

	LOG("insert succeeded");

	return true;
}
