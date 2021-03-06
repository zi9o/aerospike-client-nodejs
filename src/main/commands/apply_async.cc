/*******************************************************************************
 * Copyright 2013-2018 Aerospike, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include "client.h"
#include "async.h"
#include "conversions.h"
#include "policy.h"
#include "log.h"

using namespace v8;

NAN_METHOD(AerospikeClient::ApplyAsync)
{
	TYPE_CHECK_REQ(info[0], IsObject, "key must be an object");
	TYPE_CHECK_REQ(info[1], IsObject, "udfArgs must be an array");
	TYPE_CHECK_OPT(info[2], IsObject, "policy must be an object");
	TYPE_CHECK_REQ(info[3], IsFunction, "callback must be a function");

	AerospikeClient* client = Nan::ObjectWrap::Unwrap<AerospikeClient>(info.This());
	LogInfo* log = client->log;

	CallbackData* data = new CallbackData();
	data->client = client;
	data->callback.Reset(info[3].As<Function>());

	as_key key;
	bool key_initalized = false;
	as_policy_apply policy;
	as_policy_apply* p_policy = NULL;
	as_error err;
	as_status status;

    as_list* udf_args = NULL;
    char* udf_module = NULL;
    char* udf_function = NULL;
	bool udf_params_initialized = false;

	if (key_from_jsobject(&key, info[0]->ToObject(), log) != AS_NODE_PARAM_OK) {
		as_error_update(&err, AEROSPIKE_ERR_PARAM, "Key object invalid");
		invoke_error_callback(&err, data);
		goto Cleanup;
	}
	key_initalized = true;

	if (udfargs_from_jsobject(&udf_module, &udf_function, &udf_args, info[1]->ToObject(), log) != AS_NODE_PARAM_OK ) {
		as_error_update(&err, AEROSPIKE_ERR_PARAM, "UDF args object invalid");
		invoke_error_callback(&err, data);
		goto Cleanup;
	}
	udf_params_initialized = true;

	if (info[2]->IsObject()) {
		if (applypolicy_from_jsobject(&policy, info[2]->ToObject(), log) != AS_NODE_PARAM_OK) {
			as_error_update(&err, AEROSPIKE_ERR_PARAM, "Policy object invalid");
			invoke_error_callback(&err, data);
			goto Cleanup;
		}
		p_policy = &policy;
	}

	as_v8_debug(log, "Sending async apply command");
	status = aerospike_key_apply_async(client->as, &err, p_policy, &key, udf_module, udf_function, udf_args, async_value_listener, data, NULL, NULL);
	if (status != AEROSPIKE_OK) {
		invoke_error_callback(&err, data);
	}

Cleanup:
	if (key_initalized) as_key_destroy(&key);
	if (udf_params_initialized) {
		as_list_destroy(udf_args);
		cf_free(udf_module);
		cf_free(udf_function);
	}
}
