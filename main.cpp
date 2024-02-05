/*
 * main.cpp
 *
 *  Created on: Feb. 4, 2024
 *      Author: root
 */

//============================================================================
// Name        : v8_engine.cpp
// Author      : gabriel Helie
// Version     :
// Copyright   :
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "v8.h"
#include <libplatform/libplatform.h>
#include <sstream>
#include <fstream>
#include <cassert>

const int SUCCESS = 0;
const int ERROR = 1;

using namespace std;

//Todo: Create an event pool where each thread has its own v8 isolate
//Todo: Convert this project to a shared library
//Todo: Create a FastAPI service that can receive js scripts and call this library to compile and execute the
//		received js function and then return the function's result to the client
//Todo: Deploy on gcp compute engine
//Todo: Deploy on gcp gke

std::unique_ptr<v8::Platform> v8_platform;


void init_v8_engine() {
	v8_platform = v8::platform::NewDefaultPlatform();
	if (!v8_platform) {
		exit(ERROR);
	}
	v8::V8::InitializePlatform(v8_platform.get());
	v8::V8::Initialize();
}

class v8Context {
public:
	v8Context() {
		v8::Isolate::CreateParams create_params;
		create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
		isolate = v8::Isolate::New(create_params);
	}

	~v8Context() {
		isolate->Dispose();
		v8::V8::Dispose();
	}

	v8::Isolate* get_isolate() { return isolate; }

private:
	//isolate can only be entered by one thread
	v8::Isolate* isolate;
};

v8Context* v8_ctx = nullptr;


int read_file(const string& file_path, string& file_content) {
	ifstream f(file_path);
	if (!f) {
		return ERROR;
	}

	ostringstream ss;
	ss << f.rdbuf();
	file_content = ss.str();

	return SUCCESS;
}

int compile_js_file(const string& file_path) {
	string js_script;
	int ret = read_file(file_path, js_script);
	if (ret != SUCCESS) {
		return ret;
	}

}

void print_v8_exception(v8::TryCatch& try_catch) {
	v8::Local<v8::Value> exception = try_catch.Exception();
	v8::String::Utf8Value exception_str(v8_ctx->get_isolate(), exception);
	cout << *exception_str << endl;
}

int execute_gen_subarrays_function(const string& file_path, const string& fn_name, const vector<int>& nums, vector<vector<int>>& answer) {
	v8::Isolate::Scope isolate_scope(v8_ctx->get_isolate());
	v8::HandleScope handle_scope(v8_ctx->get_isolate());
	v8::Local<v8::Context> context = v8::Context::New(v8_ctx->get_isolate());
	v8::Context::Scope context_scope(context);

	string js_script_str;
	int ret = read_file(file_path, js_script_str);
	if (ret != SUCCESS) {
		return ret;
	}

	v8::TryCatch try_catch(v8_ctx->get_isolate());

	v8::Local<v8::String> js_script = v8::String::NewFromUtf8(v8_ctx->get_isolate(), js_script_str.c_str());
	if (js_script.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//compile the js scripts to create function global object
	v8::MaybeLocal<v8::Script> js_compiled_script_tmp = v8::Script::Compile(context, js_script);
	if (js_compiled_script_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Script> js_compiled_script = js_compiled_script_tmp.ToLocalChecked();
	if (js_compiled_script.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Value> result = js_compiled_script->Run(context).ToLocalChecked();
	if (result.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Array> js_nums = v8::Array::New(v8_ctx->get_isolate(), nums.size());
	if (js_nums.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//create v8_array from native array
	for (int i = 0; i < nums.size(); i++) {
		v8::Local<v8::Integer> js_num = v8::Integer::New(v8_ctx->get_isolate(), nums[i]);
		if (!js_nums->Set(context, i, js_num).FromMaybe(false)) {
			cout << "Failed to add to array." << endl;
			return ERROR;
		}
	}

	if (js_nums.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::MaybeLocal<v8::String> v8_fn_name = v8::String::NewFromUtf8(v8_ctx->get_isolate(),fn_name.c_str());
	if (v8_fn_name.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//get js function global object that was created when we compiled the script earlier
	v8::Local<v8::Value> fn_tmp = context->Global()->Get(context, v8_fn_name.ToLocalChecked()).ToLocalChecked();
	if (fn_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Function> fn = v8::Local<v8::Function>::Cast(fn_tmp);
	if (fn_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Value> fn_params[] = {js_nums};
	if (!fn_params || fn_params->IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::MaybeLocal<v8::Value> rValue = fn->Call(context, context->Global(), 1, fn_params);
	if (rValue.IsEmpty()) {
		print_v8_exception(try_catch);
		cout << "Gen subarrays function returned nothing." << endl;
		return ERROR;
	}

	v8::Local<v8::Value> fn_ret = rValue.ToLocalChecked();
	if (fn_ret.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	if (!fn_ret->IsArray()) {
		cout << "Gen subarrays should return a 2d array.";
		return ERROR;
	}

	v8::Local<v8::Array> fn_ret_arr = v8::Local<v8::Array>::Cast(fn_ret);
	if (fn_ret_arr.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	for (int i = 0; i < fn_ret_arr->Length(); i++) {
		v8::Local<v8::Value> tmp = fn_ret_arr->Get(context, i).ToLocalChecked();
		if (tmp.IsEmpty()) {
			print_v8_exception(try_catch);
			return ERROR;
		}

		v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(tmp);

		vector<int> sub_arr;
		for (int j = 0; j < arr->Length(); j++) {
			v8::Local<v8::Value> tmp = arr->Get(context, i).ToLocalChecked();
			if (tmp.IsEmpty()) {
				print_v8_exception(try_catch);
				return ERROR;
			}

			v8::Local<v8::Integer> num = v8::Local<v8::Integer>::Cast(tmp);
			if (num.IsEmpty()) {
				print_v8_exception(try_catch);
				return ERROR;
			}

			sub_arr.push_back(num->Value());
		}

		answer.emplace_back();
		answer.back() = sub_arr;
	}

	return SUCCESS;
}

int execute_quickselect_function(const string& file_path, const string& fn_name, const vector<int>& nums, int k, int& answer) {
	v8::Isolate::Scope isolate_scope(v8_ctx->get_isolate());
	v8::HandleScope handle_scope(v8_ctx->get_isolate());
	v8::Local<v8::Context> context = v8::Context::New(v8_ctx->get_isolate());
	v8::Context::Scope context_scope(context);

	string js_script_str;
	int ret = read_file(file_path, js_script_str);
	if (ret != SUCCESS) {
		return ret;
	}

	v8::TryCatch try_catch(v8_ctx->get_isolate());

	v8::Local<v8::String> js_script = v8::String::NewFromUtf8(v8_ctx->get_isolate(), js_script_str.c_str());
	if (js_script.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//compile the js scripts to create function global object
	v8::MaybeLocal<v8::Script> js_compiled_script_tmp = v8::Script::Compile(context, js_script);
	if (js_compiled_script_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Script> js_compiled_script = js_compiled_script_tmp.ToLocalChecked();
	if (js_compiled_script.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Value> result = js_compiled_script->Run(context).ToLocalChecked();
	if (result.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Array> js_nums = v8::Array::New(v8_ctx->get_isolate(), nums.size());
	if (js_nums.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//create v8_array from native array
	for (int i = 0; i < nums.size(); i++) {
		v8::Local<v8::Integer> js_num = v8::Integer::New(v8_ctx->get_isolate(), nums[i]);
		if (!js_nums->Set(context, i, js_num).FromMaybe(false)) {
			cout << "Failed to add to array." << endl;
			return ERROR;
		}
	}

	if (js_nums.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Value> js_k = v8::Integer::New(v8_ctx->get_isolate(), k);
	v8::Local<v8::Value> js_left = v8::Integer::New(v8_ctx->get_isolate(), 0);
	v8::Local<v8::Value> js_right = v8::Integer::New(v8_ctx->get_isolate(), nums.size()-1);

	v8::MaybeLocal<v8::String> v8_fn_name = v8::String::NewFromUtf8(v8_ctx->get_isolate(),fn_name.c_str());
	if (v8_fn_name.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//get js function global object that was created when we compiled the script earlier
	v8::Local<v8::Value> fn_tmp = context->Global()->Get(context, v8_fn_name.ToLocalChecked()).ToLocalChecked();
	if (fn_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Function> fn = v8::Local<v8::Function>::Cast(fn_tmp);
	if (fn_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Value> fn_params[] = {js_nums, js_left, js_right, js_k};
	if (!fn_params || fn_params->IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::MaybeLocal<v8::Value> rValue = fn->Call(context, context->Global(), 4, fn_params);
	if (rValue.IsEmpty()) {
		print_v8_exception(try_catch);
		cout << "quickselect function returned nothing." << endl;
		return ERROR;
	}

	v8::Local<v8::Value> fn_ret = rValue.ToLocalChecked();
	if (fn_ret.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	if (!fn_ret->IsNumber()) {
		cout << "quickselect should return a number.";
		return ERROR;
	}

	v8::Local<v8::Integer> answer_js = v8::Local<v8::Integer>::Cast(fn_ret);

	answer = answer_js->Value();

	return SUCCESS;
}

/*
 * 	Execute the popular two sum DSA question in a javascript isolate from a c++ codebase
 */
int execute_two_sum_function(const string& file_path, const string& fn_name, const vector<int>& nums, int target, vector<int>& answer) {
	v8::Isolate::Scope isolate_scope(v8_ctx->get_isolate());
	v8::HandleScope handle_scope(v8_ctx->get_isolate());
	v8::Local<v8::Context> context = v8::Context::New(v8_ctx->get_isolate());
	v8::Context::Scope context_scope(context);

	string js_script_str;
	int ret = read_file(file_path, js_script_str);
	if (ret != SUCCESS) {
		return ret;
	}

	v8::TryCatch try_catch(v8_ctx->get_isolate());

	v8::Local<v8::String> js_script = v8::String::NewFromUtf8(v8_ctx->get_isolate(), js_script_str.c_str());
	if (js_script.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//compile the js scripts to create function global object
	v8::MaybeLocal<v8::Script> js_compiled_script_tmp = v8::Script::Compile(context, js_script);
	if (js_compiled_script_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Script> js_compiled_script = js_compiled_script_tmp.ToLocalChecked();
	if (js_compiled_script.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Value> result = js_compiled_script->Run(context).ToLocalChecked();
	if (result.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Array> js_nums = v8::Array::New(v8_ctx->get_isolate(), nums.size());
	if (js_nums.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//create v8_array from native array
	for (int i = 0; i < nums.size(); i++) {
		v8::Local<v8::Integer> js_num = v8::Integer::New(v8_ctx->get_isolate(), nums[i]);
		if (!js_nums->Set(context, i, js_num).FromMaybe(false)) {
			cout << "Failed to add to array." << endl;
			return ERROR;
		}
	}

	if (js_nums.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Value> js_target = v8::Integer::New(v8_ctx->get_isolate(), target);

	v8::MaybeLocal<v8::String> v8_fn_name = v8::String::NewFromUtf8(v8_ctx->get_isolate(),fn_name.c_str());
	if (v8_fn_name.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	//get js function global object that was created when we compiled the script earlier
	v8::Local<v8::Value> fn_tmp = context->Global()->Get(context, v8_fn_name.ToLocalChecked()).ToLocalChecked();
	if (fn_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Function> fn = v8::Local<v8::Function>::Cast(fn_tmp);
	if (fn_tmp.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::Local<v8::Value> fn_params[] = {js_nums, js_target};
	if (!fn_params || fn_params->IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	v8::MaybeLocal<v8::Value> rValue = fn->Call(context, context->Global(), 2, fn_params);
	if (rValue.IsEmpty()) {
		print_v8_exception(try_catch);
		cout << "Two sum function returned nothing." << endl;
		return ERROR;
	}

	v8::Local<v8::Value> fn_ret = rValue.ToLocalChecked();
	if (fn_ret.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	if (!fn_ret->IsArray()) {
		cout << "Two sum should return an array.";
		return ERROR;
	}

	v8::Local<v8::Array> fn_ret_arr = v8::Local<v8::Array>::Cast(fn_ret);
	if (fn_ret_arr.IsEmpty()) {
		print_v8_exception(try_catch);
		return ERROR;
	}

	for (int i = 0; i < fn_ret_arr->Length(); i++) {
		v8::Local<v8::Value> tmp = fn_ret_arr->Get(context, i).ToLocalChecked();
		if (tmp.IsEmpty()) {
			print_v8_exception(try_catch);
			return ERROR;
		}

		v8::Local<v8::Integer> idx = v8::Local<v8::Integer>::Cast(tmp);
		if (idx.IsEmpty()) {
			print_v8_exception(try_catch);
			return ERROR;
		}

		answer.push_back(idx->Value());
	}

	return SUCCESS;
}

//the arguments are the js script file paths
int main(int argc, char* argv[]) {
	init_v8_engine();

	v8_ctx = new v8Context;

	argc--; //skip first arg

	if (argc != 6 != 0) {
		exit(ERROR);
	}

	//two sum
	cout << "*****************************" << endl;
	cout << "Testing two sum..." << endl;
	string two_sum_file_path = argv[1];
	string two_sum_fn_name = argv[2];
	int two_sum_status;
	vector<int> answer;

	//test one
	two_sum_status = execute_two_sum_function(two_sum_file_path, two_sum_fn_name, {1,2,3,4,5}, 9, answer);
	assert(two_sum_status == SUCCESS);
	assert(answer.size() == 2);
	assert(answer[0] == 3);
	assert(answer[1] == 4);
	cout << "The two indices that sum to target are: " << answer[0] << " and " << answer[1] << endl;

	//test two
	answer.clear();
	two_sum_status = execute_two_sum_function(two_sum_file_path, two_sum_fn_name, {1,2,3,4,5}, 20, answer);
	assert(two_sum_status == SUCCESS);
	assert(answer.size() == 0);
	cout << "No indices sum to target for this combination of params." << endl;

	cout << "Two sum test cases passed!" << endl;

	cout << "*****************************" << endl;

	//quickselect
	cout << "Testing quickselect..." << endl;
	string quickselect_file_path = argv[3];
	string quickselect_fn_name = argv[4];
	int quickselect_status;
	int quickselect_answer;

	//test one
	quickselect_status = execute_quickselect_function(quickselect_file_path, quickselect_fn_name, {1,2,3,4,5}, 2, quickselect_answer);
	assert(quickselect_status == SUCCESS);
	assert(quickselect_answer == 3);

	cout << "quickselect test cases passed!" << endl;

	cout << "*****************************" << endl;
	cout << "Testing generate subarrays..." << endl;
	string gen_subarrrays_file_path = argv[5];
	string gen_subarrrays_fn_name = argv[6];
	int gen_subarrrays_status;
	vector<vector<int>> gen_subarrrays_answer;

	//test one
	gen_subarrrays_status = execute_gen_subarrays_function(gen_subarrrays_file_path, gen_subarrrays_fn_name, {1,2,3,4,5}, gen_subarrrays_answer);
	assert(gen_subarrrays_status == SUCCESS);
	assert(gen_subarrrays_answer.size() == 15);

	cout << "generate subarrays test cases passed!" << endl;

	return 0;
}



