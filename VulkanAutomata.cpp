#define VK_USE_PLATFORM_XLIB_KHR
#define VK_API_VERSION_1_2 VK_MAKE_VERSION(1, 2, 0)
#include <cstring>
#include <ctime>
#include <chrono>
#include <cmath>
#include <time.h>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <X11/Xlib.h>
#include <vulkan/vulkan.h>
#include <regex>
#include <string>

bool 	valid 		=	1;	//	Break on error
bool 	output 		=	1;	//	?
int 	loglevel 	=	1;	//	?

//	Messaging systems designed for terminal width of 96 characters
void vr(const std::string& id, std::vector<VkResult>* reslist, VkResult res) {
//	VkResult output message
	reslist->push_back(res);
	uint32_t 	idx 		= reslist->size() - 1;
	std::string	idx_string 	= std::to_string(idx);
	uint32_t 	idx_sz		= idx_string.size();
	std::string res_string 	= std::to_string(res);
	if(idx_sz < 4) { for(int i = 0; i < 4-idx_sz; i++) { idx_string = " " + idx_string; } }
	if(output && loglevel <= 0) {
		std::cout << "  " << idx_string << ":\t" << (res==0?" ":res_string) << " \t" << id << "\n"; } }

void va(const std::string& id, std::vector<VkResult>* reslist, auto v, VkResult res) {
//	VkResult output message
	reslist->push_back(res);
	uint32_t 	idx 		= reslist->size() - 1;
	std::string	idx_string 	= std::to_string(idx);
	uint32_t 	idx_sz		= idx_string.size();
	std::string res_string 	= std::to_string(res);
	if(idx_sz < 4) { for(int i = 0; i < 4-idx_sz; i++) { idx_string = " " + idx_string; } }
	int 		padlen	= 4;
	int 		pads	= 11;
	std::string pad 	= " ";
	int 		padsize = (pads*padlen - id.size()) - 3;
	for(int i = 0; i < padsize; i++) { pad = pad + " "; }
	if(output && loglevel <= 0) {
		std::cout << "  " << idx_string << ":\t" << (res==0?" ":res_string) << " \t" << id << pad << " [" << v << "]\n"; } }

void ov(const std::string& id, auto v) {
//	Single info output message
	int 		padlen	= 4;
	int 		pads	= 10;
	std::string pad 	= " ";
	int 		padsize = (pads*padlen - id.size()) - 3;
	for(int i = 0; i < padsize; i++) { pad = pad + "."; }
	if(output && loglevel <= 1) {
		std::cout << "\tinfo:\t    " << id << pad << " [" << v << "]\n"; } }

void iv(const std::string& id, auto ov, int idx) {
//	Multiple info output message
	int 		padlen 	= 4;
	int 		pads 	= 9;
	std::string pad 	= " ";
	int 		padsize = (pads*padlen - id.size()) - 3;
	for(int i = 0; i < padsize; i++) { pad = pad + "."; }
	if(output && loglevel <= 0) {
		std::cout << "\tinfo:\t    " << idx << "\t" << id << pad << " [" << ov << "]\n"; } }

void rv(const std::string& id) {
//	Return void output message
	if(output && loglevel <= 0) {
		std::cout << "  void: \t" << id	<< "\n"; } }

void hd(const std::string& id, const std::string& msg) {
//	Header output message
	std::string bar = "";
	for(int i = 0; i < 20; i++) { bar = bar + "____"; }
	if(output && loglevel <= 0) {
		std::cout << bar << "\n " << id << "\t" << msg << "\n"; } }

void nf(auto *Vk_obj) {
//	NullFlags shorthand
	Vk_obj->pNext = NULL;
	Vk_obj->flags = 0; }

// Find a memory in `memoryTypeBitsRequirement` that includes all of `requiredProperties`
int32_t findProperties(
	const 		VkPhysicalDeviceMemoryProperties* 	pMemoryProperties,
				uint32_t 							memoryTypeBitsRequirement,
				VkMemoryPropertyFlags 				requiredProperties 			) {
    const 		uint32_t 	memoryCount = pMemoryProperties->memoryTypeCount;
    for ( uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex ) {
        const 	uint32_t 				memoryTypeBits 			= (1 << memoryIndex);
        const 	bool 					isRequiredMemoryType 	= memoryTypeBitsRequirement & memoryTypeBits;
        const 	VkMemoryPropertyFlags 	properties 				= pMemoryProperties->memoryTypes[memoryIndex].propertyFlags;
        const 	bool 					hasRequiredProperties 	= (properties & requiredProperties) == requiredProperties;
        if (isRequiredMemoryType && hasRequiredProperties) { return static_cast<int32_t>(memoryIndex); } } return -1; }

static VKAPI_ATTR VkBool32 VKAPI_CALL
//	Vulkan validation layer message output
	debugCallback(			VkDebugUtilsMessageSeverityFlagBitsEXT	messageSeverity, 
							VkDebugUtilsMessageTypeFlagsEXT			messageType, 
					const 	VkDebugUtilsMessengerCallbackDataEXT* 	pCallbackData, 
					void* 											pUserData			) {
	std::string bar = "";
	for(int i = 0; i < 20; i++) { bar = bar + "####"; }

	std::string msg 	= pCallbackData->pMessage;
	std::string msg_fmt = "";
	for(int i = 0; i < msg.size()-1; i++) {
		char chr = msg[i];
		char chrhtml = msg[i+1];
		msg_fmt = ( 
			chr == ':' && chrhtml != '/' ?
				msg_fmt+":\n\n" : ( chr == '|' ?
					msg_fmt+"\n" : msg_fmt+chr ) ); }

	msg_fmt = msg_fmt + msg[msg.size()-1];
	if(output) {
		std::cout << "\n\n" << bar << "\n " << msg_fmt << "\n" 	<< bar << "\n\n"; }
	valid = 0;
	return VK_FALSE; }

struct ShaderCodeInfo {
	std::string 		shaderFilename;
	std::vector<char>	shaderData;
	size_t 				shaderBytes;
	bool 				shaderBytesValid; };
ShaderCodeInfo getShaderCodeInfo(const std::string& filename) {
	std::ifstream 		file		(filename, std::ios::ate | std::ios::binary);
	size_t 				fileSize = 	(size_t) file.tellg();
	std::vector<char> 	buffer		(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	ShaderCodeInfo rfsc_info[1];
		rfsc_info[0].shaderFilename		= filename;
		rfsc_info[0].shaderData			= buffer;
		rfsc_info[0].shaderBytes		= buffer.size();
		rfsc_info[0].shaderBytesValid	= (rfsc_info[0].shaderBytes%4==0?1:0);
	return rfsc_info[0]; }

struct WSize {
	uint32_t 	app_w;
	uint32_t 	app_h;
	uint32_t 	divs;
	uint32_t 	mode; };
uint32_t wsize_pack(WSize ws) {
	uint32_t packed_ui32 = 
		( (uint32_t)ws.app_w		  )
	+ 	( (uint32_t)ws.app_h 	<< 12 )
	+ 	( (uint32_t)ws.divs 	<< 24 )
	+ 	( (uint32_t)ws.mode 	<< 28 );
	return packed_ui32; }

struct MInfo {
	uint32_t 	mouse_x;
	uint32_t 	mouse_y;
	uint32_t 	mouse_c;
	uint32_t 	run_cmd; };
uint32_t minfo_pack(MInfo mi) {
	uint32_t packed_ui32 = 
		( (uint32_t)mi.mouse_c	   	  )
	+ 	( (uint32_t)mi.mouse_x 	<< 4  )
	+ 	( (uint32_t)mi.mouse_y 	<< 16 )
	+ 	( (uint32_t)mi.run_cmd  << 28 );
	return packed_ui32; }

uint32_t eval2_pack(uint32_t ev[2]) {
	uint32_t packed_ui32 = 0;
	for(int i = 0; i < 2; i++) {
		packed_ui32 = packed_ui32 + ((uint32_t)ev[i] << i*16); }
	return packed_ui32; }

uint32_t eval4_pack(uint32_t ev[4]) {
	uint32_t packed_ui32 = 0;
	for(int i = 0; i < 4; i++) {
		packed_ui32 = packed_ui32 + ((uint32_t)ev[i] << i*8); }
	return packed_ui32; }

uint32_t eval8_pack(uint32_t ev[8]) {
	uint32_t packed_ui32 = 0;
	for(int i = 0; i < 8; i++) {
		packed_ui32 = packed_ui32 + ((uint32_t)ev[i] << i*4); }
	return packed_ui32; }

void eval4_unpack(uint32_t ev, uint32_t* ev32) {
		ev32[0] = uint32_t((ev      ) & uint32_t(0x000000FF) );
		ev32[1] = uint32_t((ev >>  8) & uint32_t(0x000000FF) );
		ev32[2] = uint32_t((ev >> 16) & uint32_t(0x000000FF) );
		ev32[3] = uint32_t((ev >> 24) & uint32_t(0x000000FF) ); }

void eval8_unpack(uint32_t ev, uint32_t* ev32) {
		ev32[0] = uint32_t((ev      ) & uint32_t(0x0000000F) );
		ev32[1] = uint32_t((ev >>  4) & uint32_t(0x0000000F) );
		ev32[2] = uint32_t((ev >>  8) & uint32_t(0x0000000F) );
		ev32[3] = uint32_t((ev >> 12) & uint32_t(0x0000000F) );
		ev32[4] = uint32_t((ev >> 16) & uint32_t(0x0000000F) );
		ev32[5] = uint32_t((ev >> 20) & uint32_t(0x0000000F) );
		ev32[6] = uint32_t((ev >> 24) & uint32_t(0x0000000F) );
		ev32[7] = uint32_t((ev >> 28) & uint32_t(0x0000000F) ); }

struct UniBuf {
	uint32_t wsize;
	uint32_t frame;
	uint32_t minfo;
	uint32_t i0;  uint32_t i1;  uint32_t i2;  uint32_t i3;
	uint32_t v0;  uint32_t v1;  uint32_t v2;  uint32_t v3;
	uint32_t v4;  uint32_t v5;  uint32_t v6;  uint32_t v7;
	uint32_t v8;  uint32_t v9;  uint32_t v10; uint32_t v11;
	uint32_t v12; uint32_t v13; uint32_t v14; uint32_t v15;
	uint32_t v16; uint32_t v17; uint32_t v18; uint32_t v19;
	uint32_t v20; uint32_t v21; uint32_t v22; uint32_t v23;
	uint32_t v24; uint32_t v25; uint32_t v26; uint32_t v27;
	uint32_t v28; uint32_t v29; uint32_t v30; uint32_t v31;
	uint32_t v32; uint32_t v33; uint32_t v34; uint32_t v35;
	uint32_t v36; uint32_t v37; uint32_t v38; uint32_t v39;
	uint32_t v40; uint32_t v41; uint32_t v42; uint32_t v43;
	uint32_t v44; uint32_t v45; uint32_t v46; uint32_t v47;
	float 	 scale;
	float 	 zoom; };

struct UniBuf_WF {
	uint32_t wsize;
	uint32_t frame;
	uint32_t minfo; };

struct SpecConstData {
	uint32_t sc0;  uint32_t sc1;  uint32_t sc2;  uint32_t sc3;
	uint32_t sc4;  uint32_t sc5;  uint32_t sc6;  uint32_t sc7;
	uint32_t sc8;  uint32_t sc9;  uint32_t sc10; uint32_t sc11;
	uint32_t sc12; uint32_t sc13; uint32_t sc14; uint32_t sc15;
	uint32_t sc16; uint32_t sc17; uint32_t sc18; uint32_t sc19;
	uint32_t sc20; uint32_t sc21; uint32_t sc22; uint32_t sc23;
	uint32_t sc24; uint32_t sc25; uint32_t sc26; uint32_t sc27;
	uint32_t sc28; uint32_t sc29; uint32_t sc30; uint32_t sc31;
	uint32_t sc32; uint32_t sc33; uint32_t sc34; uint32_t sc35;
	uint32_t sc36; uint32_t sc37; uint32_t sc38; uint32_t sc39;
	uint32_t sc40; uint32_t sc41; uint32_t sc42; uint32_t sc43;
	uint32_t sc44; uint32_t sc45; uint32_t sc46; uint32_t sc47; };

struct PatternConfigData_340 {
	uint32_t scd_save[32];
	uint32_t ubi_save[4];
	uint32_t ubv_save[48];
	float	 scl_save; };

struct PatternConfigData_404 {
	uint32_t scd_save[48];
	uint32_t ubi_save[4];
	uint32_t ubv_save[48];
	float	 scl_save; };

struct PatternConfigData_408 {
	uint32_t scd_save[48];
	uint32_t ubi_save[4];
	uint32_t ubv_save[48];
	float	 scl_save;
	float	 pzm_save; };

struct PatternConfigData_412 {
	uint32_t scd_save[48];
	uint32_t ubi_save[4];
	uint32_t ubv_save[48];
	uint32_t wsz_save;
	float	 scl_save;
	float	 pzm_save; };

struct PatternConfigData {
	uint32_t scd_save[48];
	uint32_t ubi_save[4];
	uint32_t ubv_save[48];
	float	 scl_save;
	float	 pzm_save; };

PatternConfigData new_pcd() {
	PatternConfigData pcd;
		for(int i = 0; i < 48; i++) { pcd.scd_save[i] = 0; }
		for(int i = 0; i <  4; i++) { pcd.ubi_save[i] = 0; }
		for(int i = 0; i < 48; i++) { pcd.ubv_save[i] = 0; }
		pcd.scl_save = 128.0;
		pcd.pzm_save =   0.0;
	return pcd; }

struct EvoData {
	uint32_t host_time;
	uint32_t save_time;
	uint32_t gen_count;
	uint32_t discarded;
	uint32_t parent_id;
	uint32_t unique_id; };

PatternConfigData load_pcd_340(std::string loadfile, int pcd_index) {
	PatternConfigData 		pcd = new_pcd();
	PatternConfigData_340 	pcd_in;
	std::ifstream fload_pcd(loadfile.c_str(), std::ios::in | std::ios::binary);
		fload_pcd.seekg(0, fload_pcd.end);
		int f_len = fload_pcd.tellg();
		fload_pcd.seekg ((pcd_index % (f_len / sizeof(pcd_in))) * sizeof(pcd_in));
		fload_pcd.read((char*)&pcd_in, sizeof(pcd_in));
	fload_pcd.close();
	for(int i = 0; i < 32; i++) { pcd.scd_save[i] = pcd_in.scd_save[i]; }
	for(int i = 0; i <  4; i++) { pcd.ubi_save[i] = pcd_in.ubi_save[i]; }
	for(int i = 0; i < 48; i++) { pcd.ubv_save[i] = pcd_in.ubv_save[i]; }
	pcd.scl_save = pcd_in.scl_save;
	return pcd; }

PatternConfigData load_pcd_404(std::string loadfile, int pcd_index) {
	PatternConfigData 		pcd = new_pcd();
	PatternConfigData_404 	pcd_in;
	std::ifstream fload_pcd(loadfile.c_str(), std::ios::in | std::ios::binary);
		fload_pcd.seekg(0, fload_pcd.end);
		int f_len = fload_pcd.tellg();
		fload_pcd.seekg ((pcd_index % (f_len / sizeof(pcd_in))) * sizeof(pcd_in));
		fload_pcd.read((char*)&pcd_in, sizeof(pcd_in));
	fload_pcd.close();
	for(int i = 0; i < 48; i++) { pcd.scd_save[i] = pcd_in.scd_save[i]; }
	for(int i = 0; i <  4; i++) { pcd.ubi_save[i] = pcd_in.ubi_save[i]; }
	for(int i = 0; i < 48; i++) { pcd.ubv_save[i] = pcd_in.ubv_save[i]; }
	pcd.scl_save = pcd_in.scl_save;
	return pcd; }

PatternConfigData load_pcd_408(std::string loadfile, int pcd_index) {
	PatternConfigData 		pcd = new_pcd();
	PatternConfigData_408 	pcd_in;
	std::ifstream fload_pcd(loadfile.c_str(), std::ios::in | std::ios::binary);
		fload_pcd.seekg(0, fload_pcd.end);
		int f_len = fload_pcd.tellg();
		fload_pcd.seekg ((pcd_index % (f_len / sizeof(pcd_in))) * sizeof(pcd_in));
		fload_pcd.read((char*)&pcd_in, sizeof(pcd_in));
	fload_pcd.close();
	for(int i = 0; i < 48; i++) { pcd.scd_save[i] = pcd_in.scd_save[i]; }
	for(int i = 0; i <  4; i++) { pcd.ubi_save[i] = pcd_in.ubi_save[i]; }
	for(int i = 0; i < 48; i++) { pcd.ubv_save[i] = pcd_in.ubv_save[i]; }
	pcd.scl_save = pcd_in.scl_save;
	pcd.pzm_save = pcd_in.pzm_save;
	return pcd; }

PatternConfigData load_pcd(uint32_t version, std::string loadfile, int pcd_index) {
	PatternConfigData pcd = new_pcd();
	if( version <= 1616459498 ) 						 { pcd = load_pcd_340(loadfile, pcd_index); }
	if( version >  1616459498 && version <= 1616831545 ) { pcd = load_pcd_404(loadfile, pcd_index); }
	if( version >  1616831545 ) 						 { pcd = load_pcd_408(loadfile, pcd_index); }
	return pcd; }

std::string PCD_out(PatternConfigData pcd) {
	std::string pcd_out = "//  PatternConfigData";
	int ar_size = sizeof(pcd.scd_save) / sizeof(pcd.scd_save[0]);
	pcd_out = pcd_out + "\n\tuint[" + std::to_string(ar_size) + "] SCD = uint[" + std::to_string(ar_size) + "] (\n\t\t";
	for(int i = 0; i < ar_size; i++) {
		pcd_out = pcd_out + std::to_string(pcd.scd_save[i]);
		pcd_out = ((i+1) % ar_size == 0) ? pcd_out + " );" : pcd_out + ", ";
		pcd_out = ((i+1) %  4 == 0) ? pcd_out + "\n\t\t" : pcd_out; }
	ar_size = sizeof(pcd.ubi_save) / sizeof(pcd.ubi_save[0]);
	pcd_out = pcd_out + "\n\tuint[" + std::to_string(ar_size) + "] UBI = uint[" + std::to_string(ar_size) + "] (\n\t\t";
	for(int i = 0; i <  ar_size; i++) {
		pcd_out = pcd_out + std::to_string(pcd.ubi_save[i]);
		pcd_out = ((i+1) %  ar_size == 0) ? pcd_out + " );" : pcd_out + ", ";
		pcd_out = ((i+1) %  4 == 0) ? pcd_out + "\n\t\t" : pcd_out; }
	ar_size = sizeof(pcd.ubv_save) / sizeof(pcd.ubv_save[0]);
	pcd_out = pcd_out + "\n\tuint[" + std::to_string(ar_size) + "] UBV = uint[" + std::to_string(ar_size) + "] (\n\t\t";
	for(int i = 0; i < ar_size; i++) {
		pcd_out = pcd_out + std::to_string(pcd.ubv_save[i]);
		pcd_out = ((i+1) % ar_size == 0) ? pcd_out + " );" : pcd_out + ", ";
		pcd_out = ((i+1) %  4 == 0) ? pcd_out + "\n\t\t" : pcd_out; }
	pcd_out = pcd_out + "\n\tfloat SCL = ";
		pcd_out = pcd_out + std::to_string(pcd.scl_save) + ";";
	pcd_out = pcd_out + "\n\tfloat PZM = ";
		pcd_out = pcd_out + std::to_string(pcd.pzm_save) + ";";
	return pcd_out + "\n"; }

void export_pcd() {
	PatternConfigData pcd;
	std::string loadfile = "fbk/save_global.vkpat";
	std::ifstream fload(loadfile.c_str(), std::ios::in | std::ios::binary);
    	fload.seekg (0, fload.end);
    int f_len = fload.tellg();
	int pcd_count = (f_len / sizeof(pcd));
	for(int i = 0; i < pcd_count; i++) {
    	fload.seekg (i * sizeof(pcd));
    	fload.read((char*)&pcd, sizeof(pcd));
		std::string pcd_file = "'out/PatternConfigData_" + std::to_string(i) + ".vkpcd'";
		std::string pcd_save = "echo '" + PCD_out(pcd) + "' > " + pcd_file;
		system(pcd_save.c_str()); }
	fload.close(); }

void find_full_config(uint32_t name_pairs[4096][2]) {

	std::cout << "Looking for configs...\n";

	std::string find_auto = "ls 'LOAD/AUTO/' > LOAD/AUTO.ls";
	system(find_auto.c_str());
	std::string find_save = "ls 'LOAD/SAVE/' > LOAD/SAVE.ls";
	system(find_save.c_str());

	std::string fs;

	std::vector <std::string> auto_fnames;
	std::string autofile = "LOAD/AUTO.ls";
	std::ifstream fload_auto(autofile.c_str(), std::ios::in | std::ios::binary);
		while(!fload_auto.eof()) {
			std::getline(fload_auto, fs);
			auto_fnames.push_back(fs); }
	fload_auto.close();
	ov("AUTO", auto_fnames.size());

	std::vector <std::string> save_fnames;
	std::string savefile = "LOAD/SAVE.ls";
	std::ifstream fload_save(savefile.c_str(), std::ios::in | std::ios::binary);
		while(!fload_save.eof()) {
			std::getline(fload_save, fs);
			save_fnames.push_back(fs); }
	fload_save.close();
	ov("SAVE", save_fnames.size());

	uint32_t timestamp_auto[auto_fnames.size()];
	for(int i = 0; i < auto_fnames.size()-1; i++) {
		timestamp_auto[i] = stoul(
				std::regex_replace(
					auto_fnames[i],
					std::regex("[^0-9]*([0-9]+).*"),
					std::string("$1") ) ); }

	uint32_t timestamp_save[save_fnames.size()];
	for(int i = 0; i < save_fnames.size()-1; i++) {
		timestamp_save[i] = stoul(
				std::regex_replace(
					save_fnames[i],
					std::regex("[^0-9]*([0-9]+).*"),
					std::string("$1") ) ); }

	uint32_t timestamp_pair[save_fnames.size()];
	for(int i = 0; i < save_fnames.size()-1; i++) {
		for(int j = 0; j < auto_fnames.size()-1; j++) {
			if(timestamp_auto[j] <= timestamp_save[i]) { timestamp_pair[i] = timestamp_auto[j]; } } }

	for(int i = 0; i < save_fnames.size()-1; i++) {
		std::string auto_load = "'./LOAD/AUTO/auto_" + std::to_string(timestamp_pair[i]) + ".frag'";
		std::string auto_used = "'./USED/auto_" + std::to_string(timestamp_save[i]) + ".frag'";
		std::string cmd_copy_auto = "cp " + auto_load + " " + auto_used;
		system(cmd_copy_auto.c_str()); }
/**/
	for(int i = 0; i < save_fnames.size()-1; i++) {
		name_pairs[i][0] = timestamp_save[i];
		name_pairs[i][1] = timestamp_pair[i]; } }

/* void reload_shader(uint32_t shader_id) { */
/* 	std::cout << "Removing SPV...\n"; */
/* 	std::string rm_cmd = "rm './app/frag_automata0000.spv'"; */
/* 	std::cout << "Building shader...\n"; */
/* 	system(rm_cmd.c_str()); */
/* 	std::string shader_source 	= "'./LOAD/AUTO/auto_" + std::to_string(shader_id) + ".frag'"; */
/* 	std::string make_shader 	= "./glslc -O " + shader_source + " -o './app/frag_automata0000.spv'"; */
/* 	system(make_shader.c_str()); */
/* 	std::cout << "Building shader... Done!\n"; } */

struct RGBA_16_UNORM_TXL {
	uint16_t r;
	uint16_t g;
	uint16_t b;
	uint16_t a; };

RGBA_16_UNORM_TXL new_RGBA_16_UNORM_TXL(uint16_t r, uint16_t g, uint16_t b, uint16_t a) {
	RGBA_16_UNORM_TXL txl;
	txl.r = r;
	txl.g = g;
	txl.b = b;
	txl.a = a;
	return txl; }

struct IMG_512_288 {
	RGBA_16_UNORM_TXL txl[512*288]; };

IMG_512_288 new_IMG_512_288() {
	IMG_512_288 img;
	for(int i = 0; i < 512*288; i++) {
		img.txl[i] = new_RGBA_16_UNORM_TXL(0, 0, 0, UINT16_MAX);
		if(i <= 512*144) { img.txl[i].r = UINT16_MAX; }
		if(i >  512*144) { img.txl[i].g = UINT16_MAX; }
		if(i%512 >  256) { img.txl[i].b = UINT16_MAX; } }
	return img; }

struct PAM_512_288 {
	char data[512*288*4]; };

PAM_512_288 loadimg(std::string loadimg) {
	PAM_512_288 imgdata;
	std::ifstream fload_img(loadimg.c_str(), std::ios::in | std::ios::binary);
		fload_img.seekg(0, fload_img.end);
		int f_len = fload_img.tellg();
		fload_img.seekg(f_len-sizeof(imgdata));
		fload_img.read((char*)&imgdata, sizeof(imgdata));
	fload_img.close();
	return imgdata; }

struct PAM_512_288_RGB {
	char data[512*288*3]; };

PAM_512_288_RGB loadimg_RGB(std::string loadimg) {
	PAM_512_288_RGB imgdata;
	std::ifstream fload_img(loadimg.c_str(), std::ios::in | std::ios::binary);
		fload_img.seekg(0, fload_img.end);
		int f_len = fload_img.tellg();
		fload_img.seekg(f_len-sizeof(imgdata));
		fload_img.read((char*)&imgdata, sizeof(imgdata));
	fload_img.close();
	return imgdata; }

IMG_512_288 PAM_to_IMG(PAM_512_288 pam_data) {
	IMG_512_288 img;
	for(int i = 0; i < 512*288; i++) {
		img.txl[i].r = pam_data.data[i*4+0] * UINT8_MAX;
		img.txl[i].g = pam_data.data[i*4+1] * UINT8_MAX;
		img.txl[i].b = pam_data.data[i*4+2] * UINT8_MAX;
		img.txl[i].a = pam_data.data[i*4+3] * UINT8_MAX; }
	return img; }

IMG_512_288 PAM_to_IMG_RGB(PAM_512_288_RGB pam_data) {
	IMG_512_288 img;
	for(int i = 0; i < 512*288; i++) {
		img.txl[i].r = pam_data.data[i*3+0] * UINT8_MAX;
		img.txl[i].g = pam_data.data[i*3+1] * UINT8_MAX;
		img.txl[i].b = pam_data.data[i*3+2] * UINT8_MAX;
		img.txl[i].a = UINT16_MAX; }
	return img; }

IMG_512_288 load_video_frame(int fidx) {
	IMG_512_288 img;
	std::string loadimg_name = "LOAD/Video/IMG" + std::to_string(fidx+1) + ".PAM";
	img = PAM_to_IMG_RGB(loadimg_RGB(loadimg_name));
	return img; }

uint32_t u32_upk(uint32_t u32, uint32_t bts, uint32_t off) { return (u32 >> off) & ((1 << bts)-1); }

uint32_t u32_flp(uint32_t u32, uint32_t off) { return u32 ^ (1 << off); }

uint32_t u32_set(uint32_t u32, uint32_t off) { return u32 | (1 << off); }
uint32_t u32_clr(uint32_t u32, uint32_t off) { return u32 & (1 << off); }

uint32_t bit_flp(uint32_t u32, uint32_t rnd) { 
	for(int i = 0; i < 32; i++) { if(rand()%rnd == 0) { u32 = u32_flp(u32, i); } } 
	return u32; }

uint32_t wrd_flp(uint32_t u32, uint32_t rnd, uint32_t wrd) { 
	for(int i = 0; i < 32; i++) { if(rand()%(rnd*wrd*((i%wrd)+1)) == 0) { u32 = u32_flp(u32, i); } } 
	return u32; }

uint32_t blk_flp(uint32_t u32, uint32_t len, uint32_t off) {
	for(int i = off; i < off+len; i++) { u32 = u32_flp(u32, i); }
	return u32; }

uint32_t blk_set(uint32_t u32, uint32_t len, uint32_t off) {
	uint32_t val = rand()%2;
	for(int i = off; i < off+len; i++) { u32 = (val==0) ? u32_clr(u32,i) : u32_set(u32,i); }
	return u32; }

void unpack32_test(uint32_t n) {
	if(loglevel != 0) { loglevel = loglevel * -1; }

	ov("N", n );
	for(int i = 0; i < 32; i++) { iv("Bit", u32_upk(n, 1, i), i ); }
	for(int i = 0; i < 32; i++) { iv("Flp", u32_flp(n, i), i); }
	if(loglevel != 0) { loglevel = loglevel * -1; } }

int main(void) {

//	Set rand() seed
	srand(time(0));

//	unpack32_test(rand()%UINT32_MAX);

//	Timestamp for file handling
	std::string timestamp = std::to_string(time(0));

	uint32_t save_version 	= stoul(timestamp);
	std::string loadfile 	= "fbk/save_global.vkpat";

/*	uint32_t save_configs[4096][2];
	for(int i = 0; i < 4096; i++) { save_configs[i][0] = 0; save_configs[i][1] = 0; }
	find_full_config(save_configs);
/**/

	PatternConfigData 		pcd;
	PatternConfigData_340 	pcd340;
	PatternConfigData_404 	pcd404;
	PatternConfigData_408 	pcd408;
	int 					load_pcd_index 	= 0;
	int 					pcd_load_size 	= 0;

//	Update old saves to current layout
/*	for(int i = 0; i < 4096; i++) { 
		if(save_configs[i][0] > 0) {
			save_version = save_configs[i][0];
			if( save_version <= 1616459498 ) 								{ pcd_load_size = sizeof(pcd340); }
			if( save_version >  1616459498 && save_version <= 1616831545 )	{ pcd_load_size = sizeof(pcd404); }
			if( save_version >  1616831545 ) 						 		{ pcd_load_size = sizeof(pcd408); }
			loadfile = "./LOAD/SAVE/save_" + std::to_string(save_configs[i][0]) + ".vkpat";
			std::ifstream fload(loadfile.c_str(), std::ios::in | std::ios::binary);
				fload.seekg (0, fload.end);
				int f_len = fload.tellg();
			fload.close();
			for(int j = 0; j < (f_len / pcd_load_size); j++) {
				pcd = load_pcd(save_version, loadfile, j);
				std::string 	savefile = "out/load_" + std::to_string(save_version) + ".vkpat";
				std::ofstream 	fout(savefile.c_str(), std::ios::out | std::ios::binary | std::ios::app);
					fout.write( (const char*)&pcd, sizeof(pcd) );
					fout.close(); } } }
/**/

//	Load an old shader config and associated save file
/*	int load_index = 0;
	loadfile = "./LOAD/SAVE/save_" + std::to_string(save_configs[load_index][0]) + ".vkpat";
	reload_shader(save_configs[load_index][1]);
	save_version = save_configs[load_index][0];
	ov("SaveFile", save_configs[load_index][0]);
/**/

//	export_pcd();

/*	for(int i = 0; i < 6221; i++) {
		pcd 	= load_pcd(save_version, loadfile, i);
		PatternConfigData pcd_tmp = load_pcd(save_version, loadfile, i);

		pcd.scd_save[24] = pcd_tmp.scd_save[28];
		pcd.scd_save[25] = pcd_tmp.scd_save[29];
		pcd.scd_save[26] = pcd_tmp.scd_save[30];
		pcd.scd_save[27] = pcd_tmp.scd_save[31];
		pcd.scd_save[28] = pcd_tmp.scd_save[24];
		pcd.scd_save[29] = pcd_tmp.scd_save[25];
		pcd.scd_save[30] = pcd_tmp.scd_save[26];
		pcd.scd_save[31] = pcd_tmp.scd_save[27];

		pcd.ubv_save[24] = pcd_tmp.ubv_save[28];
		pcd.ubv_save[25] = pcd_tmp.ubv_save[29];
		pcd.ubv_save[26] = pcd_tmp.ubv_save[30];
		pcd.ubv_save[27] = pcd_tmp.ubv_save[31];
		pcd.ubv_save[28] = pcd_tmp.ubv_save[24];
		pcd.ubv_save[29] = pcd_tmp.ubv_save[25];
		pcd.ubv_save[30] = pcd_tmp.ubv_save[26];
		pcd.ubv_save[31] = pcd_tmp.ubv_save[27];

		std::string 	savefile = "fbk/save_" + timestamp + ".vkpat.swap";
		std::ofstream 	fout_local(savefile.c_str(), std::ios::out | std::ios::binary | std::ios::app);
			fout_local.write( (const char*)&pcd, sizeof(pcd) );
		fout_local.close(); }*/

/*	for(int i = 0; i < 7914; i++) {
		pcd 	= load_pcd(save_version, loadfile, i);
		if(i < 1744) {
			for(int j = 0; j < 24; j++) {
				if(pcd.scd_save[j*2+0] <= 0) { pcd.scd_save[j*2+0] =  1; }
				if(pcd.scd_save[j*2+0] > 10) { pcd.scd_save[j*2+0] = 10; }
				if(pcd.scd_save[j*2+1] > 10) { pcd.scd_save[j*2+1] = 10; }
				if(pcd.scd_save[j*2+0] <= pcd.scd_save[j*2+1]) { pcd.scd_save[j*2+1] = pcd.scd_save[j*2+0]-1; } } }

		std::string 	savefile = "fbk/save_" + timestamp + ".vkpat.swap";
		std::ofstream 	fout_local(savefile.c_str(), std::ios::out | std::ios::binary | std::ios::app);
			fout_local.write( (const char*)&pcd, sizeof(pcd) );
		fout_local.close(); }/**/

	if(loglevel != 0) { loglevel = loglevel * -1; }

//	Make local backup: Fragment Shader (automata)
	std::string fbk_auto 	= "res/frag/" FRAG ".frag";
	std::string cp_auto		= "cp '" + fbk_auto + "' 'fbk/auto_" + timestamp +".frag'";
	system(cp_auto.c_str());

//	Make local backup: Fragment Shader (automata)
	fbk_auto 	= "res/frag/ParamUpdate.frag";
	cp_auto		= "cp '" + fbk_auto + "' 'fbk/para_" + timestamp +".frag'";
	system(cp_auto.c_str());

//	Make local backup: Render Engine
	std::string fbk_engi 	= "VulkanAutomata.cpp";
	std::string cp_engi		= "cp '" + fbk_engi + "' 'vbk/engi_" + timestamp +".cpp.bk'";
	system(cp_engi.c_str());

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "APPLICATION CONFIG");		/**/
	///////////////////////////////////////////////////

	const uint32_t 	APP_W 			= 512;		//	1920 1536 1280	768	512	384	256
	const uint32_t 	APP_H 			= 256;		//	1080 864  720	432	288	216	144
	const long 		FPS 			= 0;		//	2+
	const int 		TEST_CYCLES 	= 0;		//	0+
	const int  		SCR_BATCH		= 360;		//	120	180	300	600

	uint32_t 		PD_IDX 			= UINT32_MAX;	//	Physical Device Index
	uint32_t 		GQF_IDX 		= UINT32_MAX;	//	Graphics Queue Family Index
	uint32_t		SURF_FMT 		= UINT32_MAX;	//	Surface Format

	const long 		NS_DELAY 		= (FPS==0) ? 1 : 1000000000 / FPS;				//	Nanosecond Delay
	const float 	TRIQUAD_SCALE 	= 1.0;											//	Vertex Shader Triangle Scale
	const float 	VP_SCALE 		= TRIQUAD_SCALE + (1.0-TRIQUAD_SCALE) * 0.5;	//	Vertex Shader Viewport Scale

	const uint32_t 	VERT_FLS 		= 1;	//	Number of Vertex Shader Files
	const uint32_t 	FRAG_FLS 		= 2;	//	Number of Fragment Shader Files
	const uint32_t 	INST_EXS 		= 3;	//	Number of Vulkan Instance Extensions
	const uint32_t 	LDEV_EXS 		= 1;	//	Number of Vulkan Logical Device Extensions
	const uint32_t 	VLID_LRS 		= 1;	//	Number of Vulkan Validation Layers

//	Paths to shader files and extension names
	const char* 	filepath_vert		[VERT_FLS] =
		{	"./app/vert_TriQuad.spv" 					};
	const char* 	filepath_frag		[FRAG_FLS] =
		{	"./app/" FRAG ".spv",
		 	"./app/ParamUpdate.spv"						};
	const char* 	instance_extensions	[INST_EXS] =
		{	"VK_KHR_surface", 
			"VK_KHR_xlib_surface", 
			"VK_EXT_debug_utils"						};
	const char* 	validation_layers	[VLID_LRS] =
		{	"VK_LAYER_KHRONOS_validation" 				};
	const char* 	device_extensions	[LDEV_EXS] =
		{	"VK_KHR_swapchain"							};

//	Config Notification Messages
	ov("Window Width", 			APP_W		);
	ov("Window Height", 		APP_H		);
	ov("Render Cycles", 		TEST_CYCLES	);
	ov("FPS Target", 			FPS			);
	ov("Vertex Shaders", 		VERT_FLS	);
	ov("Fragment Shaders", 		FRAG_FLS	);

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "VULKAN INIT");			/**/
	///////////////////////////////////////////////////

//	VkResult storage
	std::vector<VkResult> vkres;
	vr("init", &vkres, VK_ERROR_UNKNOWN);

	VkApplicationInfo vka_info[1];
		vka_info[0].sType	= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vka_info[0].pNext	= NULL;
		vka_info[0].pApplicationName	= "VulkanAutomata";
		vka_info[0].applicationVersion	= 0;
		vka_info[0].pEngineName			= NULL;
		vka_info[0].engineVersion		= 0;
		vka_info[0].apiVersion			= VK_API_VERSION_1_2;

	VkInstanceCreateInfo vki_info[1];
		vki_info[0].sType 	= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	nf(&vki_info[0]);
		vki_info[0].pApplicationInfo 			= &vka_info[0];
		vki_info[0].enabledLayerCount 			= VLID_LRS;
		vki_info[0].ppEnabledLayerNames 		= validation_layers;
		vki_info[0].enabledExtensionCount 		= INST_EXS;
		vki_info[0].ppEnabledExtensionNames		= instance_extensions;

	VkInstance vki[1];
	vr("vkCreateInstance", &vkres, 
		vkCreateInstance(&vki_info[0], NULL, &vki[0]) );

	VkDebugUtilsMessengerCreateInfoEXT vkdum_info[1];
		vkdum_info[0].sType	= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	nf(&vkdum_info[0]);
		vkdum_info[0].messageSeverity	= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
										| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT 
										| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
										| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		vkdum_info[0].messageType		= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
										| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		vkdum_info[0].pfnUserCallback	= debugCallback;

	VkDebugUtilsMessengerEXT vkdum[1];
	rv("vkGetInstanceProcAddr");
	auto vkgipa_vkcdum = (PFN_vkCreateDebugUtilsMessengerEXT) 
		vkGetInstanceProcAddr( vki[0], "vkCreateDebugUtilsMessengerEXT" );
	vr("vkCreateDebugUtilsMessengerEXT", &vkres, 
		vkgipa_vkcdum(vki[0], &vkdum_info[0], NULL, &vkdum[0]) );

	uint32_t pd_cnt = UINT32_MAX;
	vr("vkEnumeratePhysicalDevices", &vkres, 
		vkEnumeratePhysicalDevices(vki[0], &pd_cnt, NULL) );
		ov("PhysicalDevices", pd_cnt);
	VkPhysicalDevice vkpd[pd_cnt];
	vr("vkEnumeratePhysicalDevices", &vkres, 
		vkEnumeratePhysicalDevices(vki[0], &pd_cnt, vkpd) );

	VkPhysicalDeviceProperties vkpd_props[pd_cnt];
	for(int i = 0; i < pd_cnt; i++) {
		rv("vkGetPhysicalDeviceProperties");
			vkGetPhysicalDeviceProperties(vkpd[i], &vkpd_props[i]);
		if(	PD_IDX == UINT32_MAX 
		&&	vkpd_props[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
			PD_IDX = i; } }

	uint32_t pdqfp_cnt = UINT32_MAX;
	rv("vkGetPhysicalDeviceQueueFamilyProperties");
		vkGetPhysicalDeviceQueueFamilyProperties(vkpd[PD_IDX], &pdqfp_cnt, NULL);
		ov("Queue Families", pdqfp_cnt);
	VkQueueFamilyProperties vkqfamprops[pdqfp_cnt];
	rv("vkGetPhysicalDeviceQueueFamilyProperties");
		vkGetPhysicalDeviceQueueFamilyProperties(vkpd[PD_IDX], &pdqfp_cnt, vkqfamprops);
	
	for(int i = 0; i < pdqfp_cnt; i++) {
		if(	GQF_IDX == UINT32_MAX 
		&&	vkqfamprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
			GQF_IDX = i; } }

	uint32_t gfxq_cnt = vkqfamprops[GQF_IDX].queueCount;
	float queue_priorities[gfxq_cnt];
	for(int i = 0; i < gfxq_cnt; i++) { queue_priorities[i] = 0.0f; }

	iv("Queues", 		gfxq_cnt, GQF_IDX							);
	iv("Queue Flags", 	vkqfamprops[GQF_IDX].queueFlags, GQF_IDX	);

	VkDeviceQueueCreateInfo vkdq_info[1];
		vkdq_info[GQF_IDX].sType	= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	nf(&vkdq_info[GQF_IDX]);
		vkdq_info[GQF_IDX].queueFamilyIndex		= GQF_IDX;
		vkdq_info[GQF_IDX].queueCount			= gfxq_cnt;
		vkdq_info[GQF_IDX].pQueuePriorities		= queue_priorities; 

	VkPhysicalDeviceFeatures vkphysdevfeat;
	rv("vkGetPhysicalDeviceFeatures");
		vkGetPhysicalDeviceFeatures(vkpd[PD_IDX], &vkphysdevfeat);

	VkDeviceCreateInfo vkld_info[1];
		vkld_info[0].sType 	= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	nf(&vkld_info[0]);
		vkld_info[0].queueCreateInfoCount 		= 1;
		vkld_info[0].pQueueCreateInfos 			= vkdq_info;
		vkld_info[0].enabledLayerCount 			= 0;
		vkld_info[0].ppEnabledLayerNames 		= NULL;
		vkld_info[0].enabledExtensionCount 		= 1;
		vkld_info[0].ppEnabledExtensionNames 	= device_extensions;
		vkld_info[0].pEnabledFeatures 			= &vkphysdevfeat;

	VkDevice vkld[1];
	vr("vkCreateDevice", &vkres, 
		vkCreateDevice(vkpd[PD_IDX], &vkld_info[0], NULL, &vkld[0]) );
		ov("VkDevice vkld[0]", vkld[0]);

	VkPhysicalDeviceMemoryProperties vkpd_memprops;
	rv("vkGetPhysicalDeviceMemoryProperties");
		vkGetPhysicalDeviceMemoryProperties(vkpd[PD_IDX], &vkpd_memprops);

	ov("memoryTypeCount", 	vkpd_memprops.memoryTypeCount					);
	for(int i = 0; i < 		vkpd_memprops.memoryTypeCount; 				i++	) {
		iv("propertyFlags", vkpd_memprops.memoryTypes[i].propertyFlags,	i	); 
		iv("heapIndex", 	vkpd_memprops.memoryTypes[i].heapIndex, 	i	); }

	ov("memoryHeapCount", 	vkpd_memprops.memoryHeapCount				);
	for(int i = 0; i < 		vkpd_memprops.memoryHeapCount; 				i++	) {
		iv("size", 			vkpd_memprops.memoryHeaps[i].size,			i	); 
		iv("flags", 		vkpd_memprops.memoryHeaps[i].flags,			i	); }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "DISPLAY");				/**/
	///////////////////////////////////////////////////

	XEvent 			xe;
	Window 			wVoid;
	int 			siVoid;
	unsigned int 	uiVoid;

	rv("XOpenDisplay");
	Display *d = 
		XOpenDisplay(NULL);
		ov("DisplayString", DisplayString(d));

	XSetWindowAttributes xswa;
	xswa.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | PointerMotionHintMask;

	Window rw = XDefaultRootWindow(d);
	rv("XCreateWindow");
	Window w = 
		XCreateWindow ( d, rw, 0, 0, APP_W, APP_H, 0, 
						CopyFromParent, CopyFromParent, CopyFromParent, 0, &xswa );
		ov("Window", w);

	XStoreName( d, w, vka_info[0].pApplicationName );

	rv("XMapWindow");
		XMapWindow(d, w);

	rv("XFlush");
		XFlush(d);

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "SURFACE");				/**/
	///////////////////////////////////////////////////

	VkXlibSurfaceCreateInfoKHR vkxls_info[1];
		vkxls_info[0].sType 	= VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	nf(&vkxls_info[0]);
		vkxls_info[0].dpy 		= d;
		vkxls_info[0].window	= w;

	VkSurfaceKHR vksurf[1];
	vr("vkCreateXlibSurfaceKHR", &vkres, 
		vkCreateXlibSurfaceKHR(vki[0], &vkxls_info[0], NULL, &vksurf[0]) );

	VkSurfaceCapabilitiesKHR vksurf_ables[1];
	vr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", &vkres, 
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkpd[PD_IDX], vksurf[0], &vksurf_ables[0]) );
		ov("minImageCount", 		vksurf_ables[0].minImageCount			);
		ov("maxImageCount", 		vksurf_ables[0].maxImageCount			);
		ov("currentExtent.width", 	vksurf_ables[0].currentExtent.width		);
		ov("currentExtent.height", 	vksurf_ables[0].currentExtent.height	);

	uint32_t pd_surf_fmt_cnt = UINT32_MAX;
	vr("vkGetPhysicalDeviceSurfaceFormatsKHR", &vkres, 
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkpd[PD_IDX], vksurf[0], 
			&pd_surf_fmt_cnt, NULL) );
		ov("PhysicalDeviceSurfaceFormats", pd_surf_fmt_cnt);
	VkSurfaceFormatKHR vksurf_fmt[pd_surf_fmt_cnt];
	vr("vkGetPhysicalDeviceSurfaceFormatsKHR", &vkres, 
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkpd[PD_IDX], vksurf[0], 
			&pd_surf_fmt_cnt, vksurf_fmt) );
		for(int i = 0; i < pd_surf_fmt_cnt; i++) { 
			iv("VkFormat", 			vksurf_fmt[i].format, 		i); 
			iv("VkColorSpaceKHR", 	vksurf_fmt[i].colorSpace, 	i);  }

	for(int i = 0; i < pd_surf_fmt_cnt; i++) {
		if(	SURF_FMT == UINT32_MAX 
		&&	vksurf_fmt[i].colorSpace 	== VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		&&	vksurf_fmt[i].format 		== VK_FORMAT_B8G8R8A8_UNORM 			) {
			SURF_FMT = i; } }

	uint32_t pd_surf_presmode_cnt = UINT32_MAX;
	vr("vkGetPhysicalDeviceSurfacePresentModesKHR", &vkres, 
		vkGetPhysicalDeviceSurfacePresentModesKHR(	vkpd[PD_IDX], vksurf[0],
													&pd_surf_presmode_cnt, NULL			) );
		ov("PhysicalDeviceSurfacePresentModes", pd_surf_fmt_cnt);
	VkPresentModeKHR vkpresmode[pd_surf_presmode_cnt];
	vr("vkGetPhysicalDeviceSurfacePresentModesKHR", &vkres, 
		vkGetPhysicalDeviceSurfacePresentModesKHR(	vkpd[PD_IDX], vksurf[0],
													&pd_surf_presmode_cnt, vkpresmode	) );
		for(int i = 0; i < pd_surf_presmode_cnt; i++) { 
			iv("VkPresentModeKHR", 	vkpresmode[i], i);  }

	VkBool32 pd_surf_supported[1];
	vr("vkGetPhysicalDeviceSurfaceSupportKHR", &vkres, 
		vkGetPhysicalDeviceSurfaceSupportKHR(	vkpd[PD_IDX], GQF_IDX, vksurf[0], 
												&pd_surf_supported[0] 				) );
		ov("Surface Supported", (pd_surf_supported[0]?"TRUE":"FALSE"));

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "SHADERS");				/**/
	///////////////////////////////////////////////////

	ShaderCodeInfo shader_info_vert[VERT_FLS];
	for(int i = 0; i < VERT_FLS; i++) {
		rv("getShaderCodeInfo");
		shader_info_vert[i] = getShaderCodeInfo(filepath_vert[i]); }

	VkShaderModuleCreateInfo vkshademod_vert_info[VERT_FLS];
	for(int i = 0; i < VERT_FLS; i++) {
		vkshademod_vert_info[i].sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	nf(&vkshademod_vert_info[i]);
		vkshademod_vert_info[i].codeSize	= shader_info_vert[i].shaderBytes;
		vkshademod_vert_info[i].pCode		= 
			reinterpret_cast<const uint32_t*>(shader_info_vert[i].shaderData.data());
		iv("Vertex shaderFilename", 	 shader_info_vert[i].shaderFilename, 					i);
		iv("Vertex shaderBytes", 		 shader_info_vert[i].shaderBytes, 						i);
		iv("Vertex shaderBytesValid", 	(shader_info_vert[i].shaderBytesValid?"TRUE":"FALSE"), 	i); }

	ShaderCodeInfo shader_info_frag[FRAG_FLS];
	for(int i = 0; i < FRAG_FLS; i++) {
		rv("getShaderCodeInfo");
		shader_info_frag[i] = getShaderCodeInfo(filepath_frag[i]); }

	VkShaderModuleCreateInfo vkshademod_frag_info[FRAG_FLS];
	for(int i = 0; i < FRAG_FLS; i++) {
		vkshademod_frag_info[i].sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	nf(&vkshademod_frag_info[i]);
		vkshademod_frag_info[i].codeSize	= shader_info_frag[i].shaderBytes;
		vkshademod_frag_info[i].pCode		= 
			reinterpret_cast<const uint32_t*>(shader_info_frag[i].shaderData.data());
		iv("Fragment shaderFilename", 	 shader_info_frag[i].shaderFilename, 					i);
		iv("Fragment shaderBytes", 		 shader_info_frag[i].shaderBytes, 						i);
		iv("Fragment shaderBytesValid", (shader_info_frag[i].shaderBytesValid?"TRUE":"FALSE"), 	i); }

	VkShaderModule vkshademod_vert[VERT_FLS];
	for(int i = 0; i < VERT_FLS; i++) {
		va("vkCreateShaderModule", &vkres, vkshademod_vert[i],
			vkCreateShaderModule(vkld[0], &vkshademod_vert_info[i], NULL, &vkshademod_vert[i]) ); }

	VkShaderModule vkshademod_frag[FRAG_FLS];
	for(int i = 0; i < FRAG_FLS; i++) {
		va("vkCreateShaderModule", &vkres, vkshademod_frag[i],
			vkCreateShaderModule(vkld[0], &vkshademod_frag_info[i], NULL, &vkshademod_frag[i]) ); }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "SWAPCHAIN");				/**/
	///////////////////////////////////////////////////

	VkSwapchainCreateInfoKHR vkswap_info[1];
		vkswap_info[0].sType	= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	nf(&vkswap_info[0]);
		vkswap_info[0].surface					= vksurf[0];
		vkswap_info[0].minImageCount			= vksurf_ables[0].minImageCount;
		vkswap_info[0].imageFormat				= vksurf_fmt[SURF_FMT].format;
		vkswap_info[0].imageColorSpace			= vksurf_fmt[SURF_FMT].colorSpace;
		vkswap_info[0].imageExtent				= vksurf_ables[0].currentExtent;
		vkswap_info[0].imageArrayLayers			= 1;
		vkswap_info[0].imageUsage				= vksurf_ables[0].supportedUsageFlags;
		vkswap_info[0].imageSharingMode			= VK_SHARING_MODE_EXCLUSIVE;
		vkswap_info[0].queueFamilyIndexCount	= 1;
		vkswap_info[0].pQueueFamilyIndices		= &GQF_IDX;
		vkswap_info[0].preTransform				= vksurf_ables[0].currentTransform;
		vkswap_info[0].compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		vkswap_info[0].presentMode				= VK_PRESENT_MODE_IMMEDIATE_KHR;
		vkswap_info[0].clipped					= VK_FALSE;
		vkswap_info[0].oldSwapchain				= VK_NULL_HANDLE;

	VkSwapchainKHR vkswap[1];
	va("vkCreateSwapchainKHR", &vkres, vkswap[0],
		vkCreateSwapchainKHR(vkld[0], &vkswap_info[0], NULL, &vkswap[0]) );

	uint32_t swap_img_cnt = UINT32_MAX;
	vr("vkGetSwapchainImagesKHR", &vkres, 
		vkGetSwapchainImagesKHR(vkld[0], vkswap[0], &swap_img_cnt, NULL) );
		ov("SwapchainImages", swap_img_cnt);

	VkImage vkswap_img[swap_img_cnt];
	vr("vkGetSwapchainImagesKHR", &vkres, 
		vkGetSwapchainImagesKHR(vkld[0], vkswap[0], &swap_img_cnt, vkswap_img) );
	ov("vkGetSwapchainImagesKHR", vkswap_img[0]);
	ov("vkGetSwapchainImagesKHR", vkswap_img[1]);

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "IMAGE LAYER PARAM");		/**/
	///////////////////////////////////////////////////

	VkExtent3D vkext3d_para;
		vkext3d_para.width 	= vksurf_ables[0].currentExtent.width;
		vkext3d_para.height = vksurf_ables[0].currentExtent.height;
		vkext3d_para.depth 	= 1;

	VkImageCreateInfo vkimg_info_para;
		vkimg_info_para.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	nf(&vkimg_info_para);
		vkimg_info_para.imageType 				= VK_IMAGE_TYPE_2D;
		vkimg_info_para.format 					= VK_FORMAT_R16G16B16A16_UNORM;
		vkimg_info_para.extent 					= vkext3d_para;
		vkimg_info_para.mipLevels 				= 1;
		vkimg_info_para.arrayLayers 			= 1;
		vkimg_info_para.samples 				= VK_SAMPLE_COUNT_1_BIT;
		vkimg_info_para.tiling 					= VK_IMAGE_TILING_OPTIMAL;
		vkimg_info_para.usage 					= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
												| VK_IMAGE_USAGE_SAMPLED_BIT
												| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
												| VK_IMAGE_USAGE_TRANSFER_DST_BIT
												| VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		vkimg_info_para.sharingMode 			= VK_SHARING_MODE_EXCLUSIVE;
		vkimg_info_para.queueFamilyIndexCount 	= 0;
		vkimg_info_para.pQueueFamilyIndices 	= NULL;
		vkimg_info_para.initialLayout 			= VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage vkimg_para[2];
	va("vkCreateImage", &vkres, vkimg_para[0],
		vkCreateImage(vkld[0], &vkimg_info_para, NULL, &vkimg_para[0]) );
	va("vkCreateImage", &vkres, vkimg_para[1],
		vkCreateImage(vkld[0], &vkimg_info_para, NULL, &vkimg_para[1]) );

	int mem_index_para[2];
	VkMemoryRequirements 	vkmemreqs_para[2];
	rv("vkGetImageMemoryRequirements");
		vkGetImageMemoryRequirements(vkld[0], vkimg_para[0], &vkmemreqs_para[0]);
		iv("memreq size", 			vkmemreqs_para[0].size, 0);
		iv("memreq alignment", 		vkmemreqs_para[0].alignment, 0);
		iv("memreq memoryTypeBits", vkmemreqs_para[0].memoryTypeBits, 0);
		mem_index_para[0] = findProperties( &vkpd_memprops, vkmemreqs_para[0].memoryTypeBits, 0x00000001 );
		iv("memoryTypeIndex", mem_index_para[0], 0);

	rv("vkGetImageMemoryRequirements");
		vkGetImageMemoryRequirements(vkld[0], vkimg_para[1], &vkmemreqs_para[1]);
		iv("memreq size", 			vkmemreqs_para[1].size, 1);
		iv("memreq alignment", 		vkmemreqs_para[1].alignment, 1);
		iv("memreq memoryTypeBits", vkmemreqs_para[1].memoryTypeBits, 1);
		mem_index_para[1] = findProperties( &vkpd_memprops, vkmemreqs_para[1].memoryTypeBits, 0x00000001 );
		iv("memoryTypeIndex", mem_index_para[1], 1);

	VkMemoryAllocateInfo vkmemalloc_info_para[2];
		vkmemalloc_info_para[0].sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkmemalloc_info_para[0].pNext			= NULL;
		vkmemalloc_info_para[0].allocationSize	= vkmemreqs_para[0].size;
		vkmemalloc_info_para[0].memoryTypeIndex	= mem_index_para[0];
		vkmemalloc_info_para[1].sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkmemalloc_info_para[1].pNext			= NULL;
		vkmemalloc_info_para[1].allocationSize	= vkmemreqs_para[1].size;
		vkmemalloc_info_para[1].memoryTypeIndex	= mem_index_para[1];

	VkDeviceMemory vkdevmemo_para[2];
	va("vkAllocateMemory", &vkres, vkdevmemo_para[0],
		vkAllocateMemory(vkld[0], &vkmemalloc_info_para[0], NULL, &vkdevmemo_para[0]) );
	va("vkAllocateMemory", &vkres, vkdevmemo_para[1],
		vkAllocateMemory(vkld[0], &vkmemalloc_info_para[1], NULL, &vkdevmemo_para[1]) );

	vr("vkBindImageMemory", &vkres, 
		vkBindImageMemory(vkld[0], vkimg_para[0], vkdevmemo_para[0], 0) );
	vr("vkBindImageMemory", &vkres, 
		vkBindImageMemory(vkld[0], vkimg_para[1], vkdevmemo_para[1], 0) );

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "IMAGE LAYER WORK");		/**/
	///////////////////////////////////////////////////

	VkExtent3D vkext3d_work;
		vkext3d_work.width 	= vksurf_ables[0].currentExtent.width;
		vkext3d_work.height = vksurf_ables[0].currentExtent.height;
		vkext3d_work.depth 	= 1;

	VkImageCreateInfo vkimg_info_work;
		vkimg_info_work.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	nf(&vkimg_info_work);
		vkimg_info_work.imageType 				= VK_IMAGE_TYPE_2D;
		vkimg_info_work.format 					= VK_FORMAT_R16G16B16A16_UNORM;
		vkimg_info_work.extent 					= vkext3d_work;
		vkimg_info_work.mipLevels 				= 1;
		vkimg_info_work.arrayLayers 			= 1;
		vkimg_info_work.samples 				= VK_SAMPLE_COUNT_1_BIT;
		vkimg_info_work.tiling 					= VK_IMAGE_TILING_OPTIMAL;
		vkimg_info_work.usage 					= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
												| VK_IMAGE_USAGE_SAMPLED_BIT
												| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
												| VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		vkimg_info_work.sharingMode 			= VK_SHARING_MODE_EXCLUSIVE;
		vkimg_info_work.queueFamilyIndexCount 	= 0;
		vkimg_info_work.pQueueFamilyIndices 	= NULL;
		vkimg_info_work.initialLayout 			= VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage vkimg_work[2];
	va("vkCreateImage", &vkres, vkimg_work[0],
		vkCreateImage(vkld[0], &vkimg_info_work, NULL, &vkimg_work[0]) );
	va("vkCreateImage", &vkres, vkimg_work[1],
		vkCreateImage(vkld[0], &vkimg_info_work, NULL, &vkimg_work[1]) );

	ov("memoryTypeCount", 	vkpd_memprops.memoryTypeCount					);
	for(int i = 0; i < 		vkpd_memprops.memoryTypeCount; 				i++	) {
		iv("propertyFlags", vkpd_memprops.memoryTypes[i].propertyFlags,	i	); 
		iv("heapIndex", 	vkpd_memprops.memoryTypes[i].heapIndex, 	i	); }

	ov("memoryHeapCount", 	vkpd_memprops.memoryHeapCount				);
	for(int i = 0; i < 		vkpd_memprops.memoryHeapCount; 				i++	) {
		iv("size", 			vkpd_memprops.memoryHeaps[i].size,			i	); 
		iv("flags", 		vkpd_memprops.memoryHeaps[i].flags,			i	); }

	int mem_index_work[2];
	VkMemoryRequirements 	vkmemreqs_work[2];
	rv("vkGetImageMemoryRequirements");
		vkGetImageMemoryRequirements(vkld[0], vkimg_work[0], &vkmemreqs_work[0]);
		iv("memreq size", 			vkmemreqs_work[0].size, 0);
		iv("memreq alignment", 		vkmemreqs_work[0].alignment, 0);
		iv("memreq memoryTypeBits", vkmemreqs_work[0].memoryTypeBits, 0);
		mem_index_work[0] = findProperties( &vkpd_memprops, vkmemreqs_work[0].memoryTypeBits, 0x00000001 );
		iv("memoryTypeIndex", mem_index_work[0], 0);

	rv("vkGetImageMemoryRequirements");
		vkGetImageMemoryRequirements(vkld[0], vkimg_work[1], &vkmemreqs_work[1]);
		iv("memreq size", 			vkmemreqs_work[1].size, 1);
		iv("memreq alignment", 		vkmemreqs_work[1].alignment, 1);
		iv("memreq memoryTypeBits", vkmemreqs_work[1].memoryTypeBits, 1);
		mem_index_work[1] = findProperties( &vkpd_memprops, vkmemreqs_work[1].memoryTypeBits, 0x00000001 );
		iv("memoryTypeIndex", mem_index_work[1], 1);

	VkMemoryAllocateInfo vkmemalloc_info_work[2];
		vkmemalloc_info_work[0].sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkmemalloc_info_work[0].pNext			= NULL;
		vkmemalloc_info_work[0].allocationSize	= vkmemreqs_work[0].size;
		vkmemalloc_info_work[0].memoryTypeIndex	= mem_index_work[0];
		vkmemalloc_info_work[1].sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkmemalloc_info_work[1].pNext			= NULL;
		vkmemalloc_info_work[1].allocationSize	= vkmemreqs_work[1].size;
		vkmemalloc_info_work[1].memoryTypeIndex	= mem_index_work[1];

	VkDeviceMemory vkdevmemo_work[2];
	va("vkAllocateMemory", &vkres, vkdevmemo_work[0],
		vkAllocateMemory(vkld[0], &vkmemalloc_info_work[0], NULL, &vkdevmemo_work[0]) );
	va("vkAllocateMemory", &vkres, vkdevmemo_work[1],
		vkAllocateMemory(vkld[0], &vkmemalloc_info_work[1], NULL, &vkdevmemo_work[1]) );

	vr("vkBindImageMemory", &vkres, 
		vkBindImageMemory(vkld[0], vkimg_work[0], vkdevmemo_work[0], 0) );
	vr("vkBindImageMemory", &vkres, 
		vkBindImageMemory(vkld[0], vkimg_work[1], vkdevmemo_work[1], 0) );

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "COMMAND BUFFERS");		/**/
	///////////////////////////////////////////////////

	VkCommandPoolCreateInfo vkcompool_info[1];
		vkcompool_info[0].sType	= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		vkcompool_info[0].pNext				= NULL;
		vkcompool_info[0].flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		vkcompool_info[0].queueFamilyIndex	= GQF_IDX;

	VkCommandPool vkcompool[1];
	va("vkCreateCommandPool", &vkres, vkcompool[0],
		vkCreateCommandPool(vkld[0], &vkcompool_info[0], NULL, &vkcompool[0]) );

	VkCommandBufferAllocateInfo vkcombuf_alloc_info[1];
		vkcombuf_alloc_info[0].sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		vkcombuf_alloc_info[0].pNext	= NULL;
		vkcombuf_alloc_info[0].commandPool			= vkcompool[0];
		vkcombuf_alloc_info[0].level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkcombuf_alloc_info[0].commandBufferCount	= swap_img_cnt;

	VkCommandBuffer vkcombuf_para_init[2];
	for(int i = 0; i < 2; i++) {
		vr("vkAllocateCommandBuffers", &vkres, 
			vkAllocateCommandBuffers(vkld[0], &vkcombuf_alloc_info[0], vkcombuf_para_init) ); }

	VkCommandBuffer vkcombuf_para[2];
	for(int i = 0; i < 2; i++) {
		vr("vkAllocateCommandBuffers", &vkres, 
			vkAllocateCommandBuffers(vkld[0], &vkcombuf_alloc_info[0], vkcombuf_para) ); }

	VkCommandBuffer vkcombuf_para_loadimg[2];
	for(int i = 0; i < 2; i++) {
		vr("vkAllocateCommandBuffers", &vkres, 
			vkAllocateCommandBuffers(vkld[0], &vkcombuf_alloc_info[0], vkcombuf_para_loadimg) ); }

	VkCommandBuffer vkcombuf_work_init[2];
	for(int i = 0; i < 2; i++) {
		vr("vkAllocateCommandBuffers", &vkres, 
			vkAllocateCommandBuffers(vkld[0], &vkcombuf_alloc_info[0], vkcombuf_work_init) ); }

	VkCommandBuffer vkcombuf_work[2];
	for(int i = 0; i < 2; i++) {
		vr("vkAllocateCommandBuffers", &vkres, 
			vkAllocateCommandBuffers(vkld[0], &vkcombuf_alloc_info[0], vkcombuf_work) ); }

	VkCommandBuffer vkcombuf_pres_init[swap_img_cnt];
	for(int i = 0; i < swap_img_cnt; i++) {
		vr("vkAllocateCommandBuffers", &vkres, 
			vkAllocateCommandBuffers(vkld[0], &vkcombuf_alloc_info[0], vkcombuf_pres_init) ); }

	VkCommandBuffer vkcombuf_pres[swap_img_cnt];
	for(int i = 0; i < swap_img_cnt; i++) {
		vr("vkAllocateCommandBuffers", &vkres, 
			vkAllocateCommandBuffers(vkld[0], &vkcombuf_alloc_info[0], vkcombuf_pres) ); }

	VkCommandBufferBeginInfo vkcombufbegin_info[swap_img_cnt];
	for(int i = 0; i < swap_img_cnt; i++) {
			vkcombufbegin_info[i].sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		nf(&vkcombufbegin_info[i]);
			vkcombufbegin_info[i].pInheritanceInfo	= NULL; }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "SYNC");					/**/
	///////////////////////////////////////////////////

	VkQueue vkq[1];
	rv("vkGetDeviceQueue");
		vkGetDeviceQueue(vkld[0], GQF_IDX, 0, &vkq[0]);

	VkSemaphoreCreateInfo vksemaph_info[1];
		vksemaph_info[0].sType	= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	nf(&vksemaph_info[0]);

	VkSemaphore vksemaph_image[1];
	va("vkCreateSemaphore", &vkres, vksemaph_image[0],
		vkCreateSemaphore(vkld[0], &vksemaph_info[0], NULL, &vksemaph_image[0]) );

	VkSemaphore vksemaph_rendr[1];
	va("vkCreateSemaphore", &vkres, vksemaph_rendr[0],
		vkCreateSemaphore(vkld[0], &vksemaph_info[0], NULL, &vksemaph_rendr[0]) );

	VkFenceCreateInfo vkfence_info[1];
		vkfence_info[0].sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	nf(&vkfence_info[0]);

	VkFence vkfence_aqimg[1];
	va("vkCreateFence", &vkres, vkfence_aqimg[0],
		vkCreateFence(vkld[0], &vkfence_info[0], NULL, &vkfence_aqimg[0]) );

	VkPipelineStageFlags qsubwait	= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	uint32_t aqimg_idx[1];

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "RENDERPASS INIT");		/**/
	///////////////////////////////////////////////////

	VkRect2D vkrect2d;
		vkrect2d.offset.x	= 0;
		vkrect2d.offset.y	= 0;
		vkrect2d.extent		= vksurf_ables[0].currentExtent;

	VkClearValue vkclearval[4];
		vkclearval[0].color 	= { 1.0f, 0.0f, 0.0f, 1.0f };
		vkclearval[1].color 	= { 0.0f, 0.0f, 1.0f, 1.0f };
		vkclearval[2].color 	= { 1.0f, 1.0f, 1.0f, 1.0f };
		vkclearval[3].color 	= { 0.0f, 1.0f, 1.0f, 1.0f };

	VkImageSubresourceRange vkimgsubrange;
		vkimgsubrange.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT;
		vkimgsubrange.baseMipLevel 		= 0;
		vkimgsubrange.levelCount 		= 1;
		vkimgsubrange.baseArrayLayer 	= 0;
		vkimgsubrange.layerCount 		= 1;

	VkImageSubresourceLayers vkimgsublayer;
		vkimgsublayer.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT;
		vkimgsublayer.mipLevel 			= 0;
		vkimgsublayer.baseArrayLayer 	= 0;
		vkimgsublayer.layerCount 		= 1;

	VkImageBlit vkimgblit;
		vkimgblit.srcSubresource 		= vkimgsublayer;
		vkimgblit.srcOffsets[0].x 		= 0;
		vkimgblit.srcOffsets[0].y 		= 0;
		vkimgblit.srcOffsets[0].z 		= 0;
		vkimgblit.srcOffsets[1].x 		= vksurf_ables[0].currentExtent.width;
		vkimgblit.srcOffsets[1].y 		= vksurf_ables[0].currentExtent.height;
		vkimgblit.srcOffsets[1].z 		= 1;
		vkimgblit.dstSubresource 		= vkimgsublayer;
		vkimgblit.dstOffsets[0].x 		= 0;
		vkimgblit.dstOffsets[0].y 		= 0;
		vkimgblit.dstOffsets[0].z 		= 0;
		vkimgblit.dstOffsets[1].x 		= vksurf_ables[0].currentExtent.width;
		vkimgblit.dstOffsets[1].y 		= vksurf_ables[0].currentExtent.height;
		vkimgblit.dstOffsets[1].z 		= 1;

	VkBufferImageCopy vkbuffimgcopy;
		vkbuffimgcopy.bufferOffset 			= 0;
		vkbuffimgcopy.bufferRowLength 		= vksurf_ables[0].currentExtent.width;
		vkbuffimgcopy.bufferImageHeight 	= vksurf_ables[0].currentExtent.height;
		vkbuffimgcopy.imageSubresource 		= vkimgsublayer;
		vkbuffimgcopy.imageOffset.x 		= 0;
		vkbuffimgcopy.imageOffset.y 		= 0;
		vkbuffimgcopy.imageOffset.z 		= 0;
		vkbuffimgcopy.imageExtent.width 	= vksurf_ables[0].currentExtent.width;
		vkbuffimgcopy.imageExtent.height 	= vksurf_ables[0].currentExtent.height;
		vkbuffimgcopy.imageExtent.depth		= 1;

	VkPipelineRasterizationStateCreateInfo vkpiperastinfo;
		vkpiperastinfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	nf(&vkpiperastinfo);
		vkpiperastinfo.depthClampEnable			= VK_FALSE;
		vkpiperastinfo.rasterizerDiscardEnable	= VK_FALSE;
		vkpiperastinfo.polygonMode				= VK_POLYGON_MODE_FILL;
		vkpiperastinfo.cullMode					= VK_CULL_MODE_NONE;
		vkpiperastinfo.frontFace				= VK_FRONT_FACE_CLOCKWISE;
		vkpiperastinfo.depthBiasEnable			= VK_FALSE;
		vkpiperastinfo.depthBiasConstantFactor	= 0.0f;
		vkpiperastinfo.depthBiasClamp			= 0.0f;
		vkpiperastinfo.depthBiasSlopeFactor		= 0.0f;
		vkpiperastinfo.lineWidth				= 1.0f;

	VkViewport vkviewport;
		vkviewport.x			= 0;
		vkviewport.y			= 0;
		vkviewport.width		= vksurf_ables[0].currentExtent.width*VP_SCALE;
		vkviewport.height		= vksurf_ables[0].currentExtent.height*VP_SCALE;
		vkviewport.minDepth		= 0.0f;
		vkviewport.maxDepth		= 1.0f;
	VkPipelineViewportStateCreateInfo vkpipeviewport_info;
		vkpipeviewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	nf(&vkpipeviewport_info);
		vkpipeviewport_info.viewportCount	= 1;
		vkpipeviewport_info.pViewports		= &vkviewport;
		vkpipeviewport_info.scissorCount	= 1;
		vkpipeviewport_info.pScissors		= &vkrect2d;

	VkPipelineMultisampleStateCreateInfo vkpipems_info;
		vkpipems_info.sType	= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	nf(&vkpipems_info);
		vkpipems_info.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
		vkpipems_info.sampleShadingEnable	= VK_FALSE;
		vkpipems_info.minSampleShading		= 0.0f;
		vkpipems_info.pSampleMask			= NULL;
		vkpipems_info.alphaToCoverageEnable	= VK_FALSE;
		vkpipems_info.alphaToOneEnable		= VK_FALSE;

	VkPipelineVertexInputStateCreateInfo vkpipevertinput_info;
		vkpipevertinput_info.sType	= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	nf(&vkpipevertinput_info);
		vkpipevertinput_info.vertexBindingDescriptionCount		= 0;
		vkpipevertinput_info.pVertexBindingDescriptions			= NULL;
		vkpipevertinput_info.vertexAttributeDescriptionCount	= 0;
		vkpipevertinput_info.pVertexAttributeDescriptions		= NULL;

	VkPipelineInputAssemblyStateCreateInfo vkpipeinputass_info;
		vkpipeinputass_info.sType	= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	nf(&vkpipeinputass_info);
		vkpipeinputass_info.topology				= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		vkpipeinputass_info.primitiveRestartEnable	= VK_FALSE;

	VkPipelineColorBlendAttachmentState vkpipecolblendatt;
		vkpipecolblendatt.blendEnable			= VK_FALSE;
		vkpipecolblendatt.srcColorBlendFactor	= VK_BLEND_FACTOR_ZERO;
		vkpipecolblendatt.dstColorBlendFactor	= VK_BLEND_FACTOR_ZERO;
		vkpipecolblendatt.colorBlendOp			= VK_BLEND_OP_ADD;
		vkpipecolblendatt.srcAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
		vkpipecolblendatt.dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
		vkpipecolblendatt.alphaBlendOp			= VK_BLEND_OP_ADD;
		vkpipecolblendatt.colorWriteMask		= 15;

	VkPipelineColorBlendStateCreateInfo vkpipecolblend;
		vkpipecolblend.sType	= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	nf(&vkpipecolblend);
		vkpipecolblend.logicOpEnable		= VK_FALSE;
		vkpipecolblend.logicOp				= VK_LOGIC_OP_NO_OP;
		vkpipecolblend.attachmentCount		= 1;
		vkpipecolblend.pAttachments			= &vkpipecolblendatt;
		vkpipecolblend.blendConstants[0]	= 1.0f;
		vkpipecolblend.blendConstants[1]	= 1.0f;
		vkpipecolblend.blendConstants[2]	= 1.0f;
		vkpipecolblend.blendConstants[3]	= 1.0f;

	const uint32_t SPCONST_ENTRIES = 48;
	size_t ui32_t_size = sizeof(uint32_t);

	SpecConstData scd;
		scd.sc0  = 0; scd.sc1  = 0; scd.sc2  = 0; scd.sc3  = 0;
		scd.sc4  = 0; scd.sc5  = 0; scd.sc6  = 0; scd.sc7  = 0;
		scd.sc8  = 0; scd.sc9  = 0; scd.sc10 = 0; scd.sc11 = 0;
		scd.sc12 = 0; scd.sc13 = 0; scd.sc14 = 0; scd.sc15 = 0;
		scd.sc16 = 0; scd.sc17 = 0; scd.sc18 = 0; scd.sc19 = 0;
		scd.sc20 = 0; scd.sc21 = 0; scd.sc22 = 0; scd.sc23 = 0;
		scd.sc24 = 0; scd.sc25 = 0; scd.sc26 = 0; scd.sc27 = 0;
		scd.sc28 = 0; scd.sc29 = 0; scd.sc30 = 0; scd.sc31 = 0;
		scd.sc32 = 0; scd.sc33 = 0; scd.sc34 = 0; scd.sc35 = 0;
		scd.sc36 = 0; scd.sc37 = 0; scd.sc38 = 0; scd.sc39 = 0;
		scd.sc40 = 0; scd.sc41 = 0; scd.sc42 = 0; scd.sc43 = 0;
		scd.sc44 = 0; scd.sc45 = 0; scd.sc46 = 0; scd.sc47 = 0;

	VkSpecializationMapEntry vkspecmapent[SPCONST_ENTRIES];
	for(int i = 0; i < SPCONST_ENTRIES; i++) {
		vkspecmapent[i].constantID 	= i;
		vkspecmapent[i].offset 		= ui32_t_size * i;
		vkspecmapent[i].size 		= ui32_t_size; }

	VkSpecializationInfo vkspecinfo;
		vkspecinfo.mapEntryCount 	= SPCONST_ENTRIES;
		vkspecinfo.pMapEntries 		= vkspecmapent;
		vkspecinfo.dataSize 		= SPCONST_ENTRIES * ui32_t_size;
		vkspecinfo.pData 			= &scd;

	VkPipelineShaderStageCreateInfo vkgfxpipe_ss_info[2];
		vkgfxpipe_ss_info[0].sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	nf(&vkgfxpipe_ss_info[0]);
		vkgfxpipe_ss_info[0].stage					= VK_SHADER_STAGE_VERTEX_BIT;
		vkgfxpipe_ss_info[0].module					= vkshademod_vert[0];
		vkgfxpipe_ss_info[0].pName					= "main";
		vkgfxpipe_ss_info[0].pSpecializationInfo	= NULL;
		vkgfxpipe_ss_info[1].sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	nf(&vkgfxpipe_ss_info[1]);
		vkgfxpipe_ss_info[1].stage					= VK_SHADER_STAGE_FRAGMENT_BIT;
		vkgfxpipe_ss_info[1].module					= vkshademod_frag[0];
		vkgfxpipe_ss_info[1].pName					= "main";
		vkgfxpipe_ss_info[1].pSpecializationInfo	= &vkspecinfo;

	WSize 	window_size;
			window_size.app_w	= APP_W;
			window_size.app_h	= APP_H;
			window_size.divs	= 1;
			window_size.mode  	= 0;

	MInfo 	mouse_info;
			mouse_info.mouse_x 	= 0;
			mouse_info.mouse_y 	= 0;
			mouse_info.mouse_c 	= 0;
			mouse_info.run_cmd 	= 0;

	VkSamplerCreateInfo vksampler_info;
		vksampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	nf(&vksampler_info);
		vksampler_info.magFilter				= VK_FILTER_NEAREST;
		vksampler_info.minFilter				= VK_FILTER_NEAREST;
		vksampler_info.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_NEAREST;
		vksampler_info.addressModeU				= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		vksampler_info.addressModeV				= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		vksampler_info.addressModeW				= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		vksampler_info.mipLodBias				= 1.0f;
		vksampler_info.anisotropyEnable			= VK_FALSE;
		vksampler_info.maxAnisotropy			= 1.0f;
		vksampler_info.compareEnable			= VK_FALSE;
		vksampler_info.compareOp				= VK_COMPARE_OP_NEVER;
		vksampler_info.minLod					= 1.0f;
		vksampler_info.maxLod					= 1.0f;
		vksampler_info.borderColor				= VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		vksampler_info.unnormalizedCoordinates	= VK_FALSE;

	VkSampler vksampler;
	va("vkCreateSampler", &vkres, vksampler,
		vkCreateSampler(vkld[0], &vksampler_info, NULL, &vksampler) );

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "RECORD PARA_INIT");		/**/
	///////////////////////////////////////////////////

	VkImageViewCreateInfo vkimgview_info_para[2];
		vkimgview_info_para[0].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	nf(&vkimgview_info_para[0]);
		vkimgview_info_para[0].image 				= vkimg_para[0];
		vkimgview_info_para[0].viewType 			= VK_IMAGE_VIEW_TYPE_2D;
		vkimgview_info_para[0].format 				= vkimg_info_para.format;
		vkimgview_info_para[0].components.r			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_para[0].components.g			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_para[0].components.b			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_para[0].components.a			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_para[0].subresourceRange 	= vkimgsubrange;
		vkimgview_info_para[1].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	nf(&vkimgview_info_para[1]);
		vkimgview_info_para[1].image 				= vkimg_para[1];
		vkimgview_info_para[1].viewType 			= VK_IMAGE_VIEW_TYPE_2D;
		vkimgview_info_para[1].format 				= vkimg_info_para.format;
		vkimgview_info_para[1].components.r			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_para[1].components.g			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_para[1].components.b			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_para[1].components.a			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_para[1].subresourceRange 	= vkimgsubrange;

	VkImageView vkimgview_para[2];
	va("vkCreateImageView", &vkres, vkimgview_para[0],
		vkCreateImageView(vkld[0], &vkimgview_info_para[0], NULL, &vkimgview_para[0]) );
	va("vkCreateImageView", &vkres, vkimgview_para[1],
		vkCreateImageView(vkld[0], &vkimgview_info_para[1], NULL, &vkimgview_para[1]) );

	VkImageMemoryBarrier vkimgmembar_para_und2sro[2];
		vkimgmembar_para_und2sro[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_para_und2sro[0].pNext 					= NULL;
		vkimgmembar_para_und2sro[0].srcAccessMask 			= 0;
		vkimgmembar_para_und2sro[0].dstAccessMask 			= 0;
		vkimgmembar_para_und2sro[0].oldLayout 				= VK_IMAGE_LAYOUT_UNDEFINED;
		vkimgmembar_para_und2sro[0].newLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_para_und2sro[0].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_und2sro[0].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_und2sro[0].image 					= vkimg_para[0];
		vkimgmembar_para_und2sro[0].subresourceRange 		= vkimgsubrange;
		vkimgmembar_para_und2sro[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_para_und2sro[1].pNext 					= NULL;
		vkimgmembar_para_und2sro[1].srcAccessMask 			= 0;
		vkimgmembar_para_und2sro[1].dstAccessMask 			= 0;
		vkimgmembar_para_und2sro[1].oldLayout 				= VK_IMAGE_LAYOUT_UNDEFINED;
		vkimgmembar_para_und2sro[1].newLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_para_und2sro[1].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_und2sro[1].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_und2sro[1].image 					= vkimg_para[1];
		vkimgmembar_para_und2sro[1].subresourceRange 		= vkimgsubrange;

	for(int i = 0; i < 2; i++) {
		vr("vkBeginCommandBuffer", &vkres, 
			vkBeginCommandBuffer(vkcombuf_para_init[i], &vkcombufbegin_info[i]) );

			rv("vkCmdPipelineBarrier");
				vkCmdPipelineBarrier (
					vkcombuf_para_init[i],
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT,
					0, NULL, 0, NULL,
					1, &vkimgmembar_para_und2sro[i] );

		vr("vkEndCommandBuffer", &vkres, 
			vkEndCommandBuffer(vkcombuf_para_init[i]) ); }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "SUBMIT PARA_INIT");		/**/
	///////////////////////////////////////////////////

	for(int i = 0; i < 2; i++) {
		if(valid) {
			rv("vkcombuf_para_init");
			VkSubmitInfo vksub_info[1];
				vksub_info[0].sType	= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				vksub_info[0].pNext					= NULL;
				vksub_info[0].waitSemaphoreCount	= 0;
				vksub_info[0].pWaitSemaphores		= NULL;
				vksub_info[0].pWaitDstStageMask		= NULL;
				vksub_info[0].commandBufferCount	= 1;
				vksub_info[0].pCommandBuffers		= &vkcombuf_para_init[i];
				vksub_info[0].signalSemaphoreCount	= 0;
				vksub_info[0].pSignalSemaphores		= NULL;
			vr("vkQueueSubmit", &vkres, 
				vkQueueSubmit(vkq[0], 1, vksub_info, VK_NULL_HANDLE) ); } }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "RECORD WORK_INIT");		/**/
	///////////////////////////////////////////////////

	VkImageViewCreateInfo vkimgview_info_work[2];
		vkimgview_info_work[0].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	nf(&vkimgview_info_work[0]);
		vkimgview_info_work[0].image 				= vkimg_work[0];
		vkimgview_info_work[0].viewType 			= VK_IMAGE_VIEW_TYPE_2D;
		vkimgview_info_work[0].format 				= vkimg_info_work.format;
		vkimgview_info_work[0].components.r			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_work[0].components.g			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_work[0].components.b			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_work[0].components.a			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_work[0].subresourceRange 	= vkimgsubrange;
		vkimgview_info_work[1].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	nf(&vkimgview_info_work[1]);
		vkimgview_info_work[1].image 				= vkimg_work[1];
		vkimgview_info_work[1].viewType 			= VK_IMAGE_VIEW_TYPE_2D;
		vkimgview_info_work[1].format 				= vkimg_info_work.format;
		vkimgview_info_work[1].components.r			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_work[1].components.g			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_work[1].components.b			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_work[1].components.a			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info_work[1].subresourceRange 	= vkimgsubrange;

	VkImageView vkimgview_work[2];
	va("vkCreateImageView", &vkres, vkimgview_work[0],
		vkCreateImageView(vkld[0], &vkimgview_info_work[0], NULL, &vkimgview_work[0]) );
	va("vkCreateImageView", &vkres, vkimgview_work[1],
		vkCreateImageView(vkld[0], &vkimgview_info_work[1], NULL, &vkimgview_work[1]) );

	VkImageMemoryBarrier vkimgmembar_work_und2sro[2];
		vkimgmembar_work_und2sro[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_work_und2sro[0].pNext 					= NULL;
		vkimgmembar_work_und2sro[0].srcAccessMask 			= 0;
		vkimgmembar_work_und2sro[0].dstAccessMask 			= 0;
		vkimgmembar_work_und2sro[0].oldLayout 				= VK_IMAGE_LAYOUT_UNDEFINED;
		vkimgmembar_work_und2sro[0].newLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_work_und2sro[0].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_work_und2sro[0].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_work_und2sro[0].image 					= vkimg_work[0];
		vkimgmembar_work_und2sro[0].subresourceRange 		= vkimgsubrange;
		vkimgmembar_work_und2sro[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_work_und2sro[1].pNext 					= NULL;
		vkimgmembar_work_und2sro[1].srcAccessMask 			= 0;
		vkimgmembar_work_und2sro[1].dstAccessMask 			= 0;
		vkimgmembar_work_und2sro[1].oldLayout 				= VK_IMAGE_LAYOUT_UNDEFINED;
		vkimgmembar_work_und2sro[1].newLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_work_und2sro[1].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_work_und2sro[1].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_work_und2sro[1].image 					= vkimg_work[1];
		vkimgmembar_work_und2sro[1].subresourceRange 		= vkimgsubrange;

	for(int i = 0; i < 2; i++) {
		vr("vkBeginCommandBuffer", &vkres, 
			vkBeginCommandBuffer(vkcombuf_work_init[i], &vkcombufbegin_info[i]) );

			rv("vkCmdPipelineBarrier");
				vkCmdPipelineBarrier (
					vkcombuf_work_init[i],
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT,
					0, NULL, 0, NULL,
					1, &vkimgmembar_work_und2sro[i] );

		vr("vkEndCommandBuffer", &vkres, 
			vkEndCommandBuffer(vkcombuf_work_init[i]) ); }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "SUBMIT WORK_INIT");		/**/
	///////////////////////////////////////////////////

	for(int i = 0; i < 2; i++) {
		if(valid) {
			rv("vkcombuf_work_init");
			VkSubmitInfo vksub_info[1];
				vksub_info[0].sType	= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				vksub_info[0].pNext					= NULL;
				vksub_info[0].waitSemaphoreCount	= 0;
				vksub_info[0].pWaitSemaphores		= NULL;
				vksub_info[0].pWaitDstStageMask		= NULL;
				vksub_info[0].commandBufferCount	= 1;
				vksub_info[0].pCommandBuffers		= &vkcombuf_work_init[i];
				vksub_info[0].signalSemaphoreCount	= 0;
				vksub_info[0].pSignalSemaphores		= NULL;
			vr("vkQueueSubmit", &vkres, 
				vkQueueSubmit(vkq[0], 1, vksub_info, VK_NULL_HANDLE) ); } }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "RECORD PARA_LOOP");		/**/
	///////////////////////////////////////////////////

//	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO	TODO

	VkAttachmentDescription vkattdesc_para[2];
		vkattdesc_para[0].flags 			= 0;
		vkattdesc_para[0].format 			= vkimg_info_para.format;
		vkattdesc_para[0].samples 			= VK_SAMPLE_COUNT_1_BIT;
		vkattdesc_para[0].loadOp 			= VK_ATTACHMENT_LOAD_OP_LOAD;
		vkattdesc_para[0].storeOp 			= VK_ATTACHMENT_STORE_OP_STORE;
		vkattdesc_para[0].stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkattdesc_para[0].stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkattdesc_para[0].initialLayout 	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkattdesc_para[0].finalLayout 		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkattdesc_para[1].flags 			= 0;
		vkattdesc_para[1].format 			= vkimg_info_para.format;
		vkattdesc_para[1].samples 			= VK_SAMPLE_COUNT_1_BIT;
		vkattdesc_para[1].loadOp 			= VK_ATTACHMENT_LOAD_OP_LOAD;
		vkattdesc_para[1].storeOp 			= VK_ATTACHMENT_STORE_OP_STORE;
		vkattdesc_para[1].stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkattdesc_para[1].stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkattdesc_para[1].initialLayout 	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkattdesc_para[1].finalLayout 		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference vkattref_para;
		vkattref_para.attachment 	= 0;
		vkattref_para.layout 		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription vksubpassdesc_para;
		vksubpassdesc_para.flags 						= 0;
		vksubpassdesc_para.pipelineBindPoint 			= VK_PIPELINE_BIND_POINT_GRAPHICS;
		vksubpassdesc_para.inputAttachmentCount 		= 0;
		vksubpassdesc_para.pInputAttachments 			= NULL;
		vksubpassdesc_para.colorAttachmentCount 		= 1;
		vksubpassdesc_para.pColorAttachments 			= &vkattref_para;
		vksubpassdesc_para.pResolveAttachments 			= NULL;
		vksubpassdesc_para.pDepthStencilAttachment 		= NULL;
		vksubpassdesc_para.preserveAttachmentCount 		= 0;
		vksubpassdesc_para.pPreserveAttachments 		= NULL;

	VkRenderPassCreateInfo vkrendpass_info_para[2];
		vkrendpass_info_para[0].sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	nf(&vkrendpass_info_para[0]);
		vkrendpass_info_para[0].attachmentCount 	= 1;
		vkrendpass_info_para[0].pAttachments 		= &vkattdesc_para[0];
		vkrendpass_info_para[0].subpassCount 		= 1;
		vkrendpass_info_para[0].pSubpasses 			= &vksubpassdesc_para;
		vkrendpass_info_para[0].dependencyCount 	= 0;
		vkrendpass_info_para[0].pDependencies 		= NULL;
		vkrendpass_info_para[1].sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	nf(&vkrendpass_info_para[1]);
		vkrendpass_info_para[1].attachmentCount 	= 1;
		vkrendpass_info_para[1].pAttachments 		= &vkattdesc_para[1];
		vkrendpass_info_para[1].subpassCount 		= 1;
		vkrendpass_info_para[1].pSubpasses 			= &vksubpassdesc_para;
		vkrendpass_info_para[1].dependencyCount 	= 0;
		vkrendpass_info_para[1].pDependencies 		= NULL;

	VkRenderPass vkrendpass_para[2];
	va("vkCreateRenderPass", &vkres, vkrendpass_para[0],
		vkCreateRenderPass(vkld[0], &vkrendpass_info_para[0], NULL, &vkrendpass_para[0]) );
	va("vkCreateRenderPass", &vkres, vkrendpass_para[1],
		vkCreateRenderPass(vkld[0], &vkrendpass_info_para[1], NULL, &vkrendpass_para[1]) );

	VkFramebufferCreateInfo vkframebuff_info_para[2];
		vkframebuff_info_para[0].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	nf(&vkframebuff_info_para[0]);
		vkframebuff_info_para[0].renderPass 		= vkrendpass_para[0];
		vkframebuff_info_para[0].attachmentCount 	= 1;
		vkframebuff_info_para[0].pAttachments 		= &vkimgview_para[0];
		vkframebuff_info_para[0].width 				= vksurf_ables[0].currentExtent.width;
		vkframebuff_info_para[0].height 			= vksurf_ables[0].currentExtent.height;
		vkframebuff_info_para[0].layers 			= 1;
		vkframebuff_info_para[1].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	nf(&vkframebuff_info_para[1]);
		vkframebuff_info_para[1].renderPass 		= vkrendpass_para[1];
		vkframebuff_info_para[1].attachmentCount 	= 1;
		vkframebuff_info_para[1].pAttachments 		= &vkimgview_para[1];
		vkframebuff_info_para[1].width 				= vksurf_ables[0].currentExtent.width;
		vkframebuff_info_para[1].height 			= vksurf_ables[0].currentExtent.height;
		vkframebuff_info_para[1].layers 			= 1;

	VkFramebuffer vkframebuff_para[2];
	va("vkCreateFramebuffer", &vkres, vkframebuff_para[0],
		vkCreateFramebuffer(vkld[0], &vkframebuff_info_para[0], NULL, &vkframebuff_para[0]) );
	va("vkCreateFramebuffer", &vkres, vkframebuff_para[1],
		vkCreateFramebuffer(vkld[0], &vkframebuff_info_para[1], NULL, &vkframebuff_para[1]) );

	VkRenderPassBeginInfo vkrpbegininfo_para[2];
		vkrpbegininfo_para[0].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkrpbegininfo_para[0].pNext 				= NULL;
		vkrpbegininfo_para[0].renderPass 			= vkrendpass_para[0];
		vkrpbegininfo_para[0].framebuffer 			= vkframebuff_para[0];
		vkrpbegininfo_para[0].renderArea 			= vkrect2d;
		vkrpbegininfo_para[0].clearValueCount 		= 1;
		vkrpbegininfo_para[0].pClearValues 			= &vkclearval[2];
		vkrpbegininfo_para[1].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkrpbegininfo_para[1].pNext 				= NULL;
		vkrpbegininfo_para[1].renderPass 			= vkrendpass_para[1];
		vkrpbegininfo_para[1].framebuffer 			= vkframebuff_para[1];
		vkrpbegininfo_para[1].renderArea 			= vkrect2d;
		vkrpbegininfo_para[1].clearValueCount 		= 1;
		vkrpbegininfo_para[1].pClearValues 			= &vkclearval[3];

	const int PARAIMGS = 2;

	VkDescriptorSetLayoutBinding vkdescsetlaybind_para[3];
		vkdescsetlaybind_para[0].binding				= 0;
		vkdescsetlaybind_para[0].descriptorType			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vkdescsetlaybind_para[0].descriptorCount		= 1;
		vkdescsetlaybind_para[0].stageFlags				= VK_SHADER_STAGE_FRAGMENT_BIT;
		vkdescsetlaybind_para[0].pImmutableSamplers		= NULL;
		vkdescsetlaybind_para[1].binding				= 1;
		vkdescsetlaybind_para[1].descriptorType			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		vkdescsetlaybind_para[1].descriptorCount		= 1;
		vkdescsetlaybind_para[1].stageFlags				= VK_SHADER_STAGE_FRAGMENT_BIT;
		vkdescsetlaybind_para[1].pImmutableSamplers		= NULL;
		vkdescsetlaybind_para[2].binding				= 2;
		vkdescsetlaybind_para[2].descriptorType			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		vkdescsetlaybind_para[2].descriptorCount		= 1;
		vkdescsetlaybind_para[2].stageFlags				= VK_SHADER_STAGE_FRAGMENT_BIT;
		vkdescsetlaybind_para[2].pImmutableSamplers		= NULL;

	VkDescriptorSetLayoutCreateInfo vkdescset_info_para;
		vkdescset_info_para.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	nf(&vkdescset_info_para);
		vkdescset_info_para.bindingCount	= 3;
		vkdescset_info_para.pBindings		= vkdescsetlaybind_para;

	VkDescriptorSetLayout vkdescsetlay_para;
	va("vkCreateDescriptorSetLayout", &vkres, vkdescsetlay_para,
		vkCreateDescriptorSetLayout(vkld[0], &vkdescset_info_para, NULL, &vkdescsetlay_para) );

	VkDescriptorPoolSize vkdescpoolsize_para[3];
		vkdescpoolsize_para[0].type 			= vkdescsetlaybind_para[0].descriptorType;
		vkdescpoolsize_para[0].descriptorCount 	= 7;
		vkdescpoolsize_para[1].type 			= vkdescsetlaybind_para[1].descriptorType;
		vkdescpoolsize_para[1].descriptorCount 	= 3;
		vkdescpoolsize_para[2].type 			= vkdescsetlaybind_para[2].descriptorType;
		vkdescpoolsize_para[2].descriptorCount 	= 1;

	VkDescriptorPoolCreateInfo vkdescpool_info_para[3];
		vkdescpool_info_para[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	nf(&vkdescpool_info_para[0]);
		vkdescpool_info_para[0].maxSets 		= 1;
		vkdescpool_info_para[0].poolSizeCount 	= 1;
		vkdescpool_info_para[0].pPoolSizes 		= &vkdescpoolsize_para[0];
		vkdescpool_info_para[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	nf(&vkdescpool_info_para[1]);
		vkdescpool_info_para[1].maxSets 		= 1;
		vkdescpool_info_para[1].poolSizeCount 	= 1;
		vkdescpool_info_para[1].pPoolSizes 		= &vkdescpoolsize_para[1];
		vkdescpool_info_para[2].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	nf(&vkdescpool_info_para[2]);
		vkdescpool_info_para[2].maxSets 		= 1;
		vkdescpool_info_para[2].poolSizeCount 	= 1;
		vkdescpool_info_para[2].pPoolSizes 		= &vkdescpoolsize_para[2];

	VkDescriptorPool vkdescpool_para[3];
	va("vkCreateDescriptorPool", &vkres, vkdescpool_para[0],
		vkCreateDescriptorPool(vkld[0], &vkdescpool_info_para[0], NULL, &vkdescpool_para[0]) );
	va("vkCreateDescriptorPool", &vkres, vkdescpool_para[1],
		vkCreateDescriptorPool(vkld[0], &vkdescpool_info_para[1], NULL, &vkdescpool_para[1]) );
	va("vkCreateDescriptorPool", &vkres, vkdescpool_para[2],
		vkCreateDescriptorPool(vkld[0], &vkdescpool_info_para[2], NULL, &vkdescpool_para[2]) );

	VkDescriptorSetAllocateInfo vkdescsetallo_info_para[3];
		vkdescsetallo_info_para[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		vkdescsetallo_info_para[0].pNext 				= NULL;
		vkdescsetallo_info_para[0].descriptorPool 		= vkdescpool_para[0];
		vkdescsetallo_info_para[0].descriptorSetCount 	= 1;
		vkdescsetallo_info_para[0].pSetLayouts 			= &vkdescsetlay_para;
		vkdescsetallo_info_para[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		vkdescsetallo_info_para[1].pNext 				= NULL;
		vkdescsetallo_info_para[1].descriptorPool 		= vkdescpool_para[1];
		vkdescsetallo_info_para[1].descriptorSetCount 	= 1;
		vkdescsetallo_info_para[1].pSetLayouts 			= &vkdescsetlay_para;
		vkdescsetallo_info_para[2].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		vkdescsetallo_info_para[2].pNext 				= NULL;
		vkdescsetallo_info_para[2].descriptorPool 		= vkdescpool_para[1];
		vkdescsetallo_info_para[2].descriptorSetCount 	= 1;
		vkdescsetallo_info_para[2].pSetLayouts 			= &vkdescsetlay_para;

	VkDescriptorSet vkdescset_para[3];
	va("vkAllocateDescriptorSets", &vkres, vkdescset_para[0],
		vkAllocateDescriptorSets(vkld[0], &vkdescsetallo_info_para[0], &vkdescset_para[0]) );
	va("vkAllocateDescriptorSets", &vkres, vkdescset_para[1],
		vkAllocateDescriptorSets(vkld[0], &vkdescsetallo_info_para[1], &vkdescset_para[1]) );
	va("vkAllocateDescriptorSets", &vkres, vkdescset_para[2],
		vkAllocateDescriptorSets(vkld[0], &vkdescsetallo_info_para[2], &vkdescset_para[2]) );

	VkDescriptorImageInfo vkdescimg_info_para_sample_0[2];
		vkdescimg_info_para_sample_0[0].sampler			= vksampler;
		vkdescimg_info_para_sample_0[0].imageView		= vkimgview_para[1]; // Reference the other image
		vkdescimg_info_para_sample_0[0].imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkdescimg_info_para_sample_0[1].sampler			= vksampler;
		vkdescimg_info_para_sample_0[1].imageView		= vkimgview_para[0]; // Reference the other image
		vkdescimg_info_para_sample_0[1].imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo vkdescimg_info_para_sample_1[2];
		vkdescimg_info_para_sample_1[0].sampler			= vksampler;
		vkdescimg_info_para_sample_1[0].imageView		= vkimgview_work[1]; // Reference the other image
		vkdescimg_info_para_sample_1[0].imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkdescimg_info_para_sample_1[1].sampler			= vksampler;
		vkdescimg_info_para_sample_1[1].imageView		= vkimgview_work[0]; // Reference the other image
		vkdescimg_info_para_sample_1[1].imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet vkwritedescset_para_sample_0[2];
		vkwritedescset_para_sample_0[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_para_sample_0[0].pNext 				= NULL;
		vkwritedescset_para_sample_0[0].dstSet 				= vkdescset_para[0];
		vkwritedescset_para_sample_0[0].dstBinding 			= 1;
		vkwritedescset_para_sample_0[0].dstArrayElement 	= 0;
		vkwritedescset_para_sample_0[0].descriptorCount 	= 1;
		vkwritedescset_para_sample_0[0].descriptorType 		= vkdescsetlaybind_para[1].descriptorType;
		vkwritedescset_para_sample_0[0].pImageInfo 			= &vkdescimg_info_para_sample_0[0];
		vkwritedescset_para_sample_0[0].pBufferInfo 		= NULL;
		vkwritedescset_para_sample_0[0].pTexelBufferView 	= NULL;
		vkwritedescset_para_sample_0[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_para_sample_0[1].pNext 				= NULL;
		vkwritedescset_para_sample_0[1].dstSet 				= vkdescset_para[1];
		vkwritedescset_para_sample_0[1].dstBinding 			= 1;
		vkwritedescset_para_sample_0[1].dstArrayElement 	= 0;
		vkwritedescset_para_sample_0[1].descriptorCount 	= 1;
		vkwritedescset_para_sample_0[1].descriptorType 		= vkdescsetlaybind_para[1].descriptorType;
		vkwritedescset_para_sample_0[1].pImageInfo 			= &vkdescimg_info_para_sample_0[1];
		vkwritedescset_para_sample_0[1].pBufferInfo 		= NULL;
		vkwritedescset_para_sample_0[1].pTexelBufferView 	= NULL;

	VkWriteDescriptorSet vkwritedescset_para_sample_1[2];
		vkwritedescset_para_sample_1[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_para_sample_1[0].pNext 				= NULL;
		vkwritedescset_para_sample_1[0].dstSet 				= vkdescset_para[0];
		vkwritedescset_para_sample_1[0].dstBinding 			= 2;
		vkwritedescset_para_sample_1[0].dstArrayElement 	= 0;
		vkwritedescset_para_sample_1[0].descriptorCount 	= 1;
		vkwritedescset_para_sample_1[0].descriptorType 		= vkdescsetlaybind_para[2].descriptorType;
		vkwritedescset_para_sample_1[0].pImageInfo 			= &vkdescimg_info_para_sample_1[0];
		vkwritedescset_para_sample_1[0].pBufferInfo 		= NULL;
		vkwritedescset_para_sample_1[0].pTexelBufferView 	= NULL;
		vkwritedescset_para_sample_1[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_para_sample_1[1].pNext 				= NULL;
		vkwritedescset_para_sample_1[1].dstSet 				= vkdescset_para[1];
		vkwritedescset_para_sample_1[1].dstBinding 			= 2;
		vkwritedescset_para_sample_1[1].dstArrayElement 	= 0;
		vkwritedescset_para_sample_1[1].descriptorCount 	= 1;
		vkwritedescset_para_sample_1[1].descriptorType 		= vkdescsetlaybind_para[2].descriptorType;
		vkwritedescset_para_sample_1[1].pImageInfo 			= &vkdescimg_info_para_sample_1[1];
		vkwritedescset_para_sample_1[1].pBufferInfo 		= NULL;
		vkwritedescset_para_sample_1[1].pTexelBufferView 	= NULL;

	VkDeviceSize vkdevsize_para;
		vkdevsize_para = sizeof(UniBuf_WF);
	ov("UniBuf_WF size", vkdevsize_para);

	VkBufferCreateInfo vkbuff_info_para;
		vkbuff_info_para.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	nf(&vkbuff_info_para);
		vkbuff_info_para.size 						= vkdevsize_para;
		vkbuff_info_para.usage 						= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		vkbuff_info_para.sharingMode 				= VK_SHARING_MODE_EXCLUSIVE;
		vkbuff_info_para.queueFamilyIndexCount 		= 1;
		vkbuff_info_para.pQueueFamilyIndices 		= &GQF_IDX;

	VkBuffer vkbuff_para;
	va("vkCreateBuffer", &vkres, vkbuff_para,
		vkCreateBuffer(vkld[0], &vkbuff_info_para, NULL, &vkbuff_para) );

	VkDescriptorBufferInfo vkDescBuff_info_para;
		vkDescBuff_info_para.buffer 	= vkbuff_para;
		vkDescBuff_info_para.offset 	= 0;
		vkDescBuff_info_para.range 		= VK_WHOLE_SIZE;

	VkWriteDescriptorSet vkwritedescset_ub_para[2];
		vkwritedescset_ub_para[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_ub_para[0].pNext 				= NULL;
		vkwritedescset_ub_para[0].dstSet 				= vkdescset_para[0];
		vkwritedescset_ub_para[0].dstBinding 			= 0;
		vkwritedescset_ub_para[0].dstArrayElement 		= 0;
		vkwritedescset_ub_para[0].descriptorCount 		= 1;
		vkwritedescset_ub_para[0].descriptorType 		= vkdescsetlaybind_para[0].descriptorType;
		vkwritedescset_ub_para[0].pImageInfo 			= NULL;
		vkwritedescset_ub_para[0].pBufferInfo 			= &vkDescBuff_info_para;
		vkwritedescset_ub_para[0].pTexelBufferView 		= NULL;
		vkwritedescset_ub_para[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_ub_para[1].pNext 				= NULL;
		vkwritedescset_ub_para[1].dstSet 				= vkdescset_para[1];
		vkwritedescset_ub_para[1].dstBinding 			= 0;
		vkwritedescset_ub_para[1].dstArrayElement 		= 0;
		vkwritedescset_ub_para[1].descriptorCount 		= 1;
		vkwritedescset_ub_para[1].descriptorType 		= vkdescsetlaybind_para[0].descriptorType;
		vkwritedescset_ub_para[1].pImageInfo 			= NULL;
		vkwritedescset_ub_para[1].pBufferInfo 			= &vkDescBuff_info_para;
		vkwritedescset_ub_para[1].pTexelBufferView 		= NULL;

	int mem_index_ub_para = UINT32_MAX;
	for(int i = 0; i < vkpd_memprops.memoryTypeCount; i++) {
		if( vkpd_memprops.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		&&	mem_index_ub_para == UINT32_MAX ) {
			mem_index_ub_para = i; } }
	ov("mem_index_ub_para", mem_index_ub_para);

	VkMemoryRequirements vkmemreqs_ub_para;
	rv("vkGetBufferMemoryRequirements");
		vkGetBufferMemoryRequirements(vkld[0], vkbuff_para, &vkmemreqs_ub_para);
		ov("memreq size", 			vkmemreqs_ub_para.size);
		ov("memreq alignment", 		vkmemreqs_ub_para.alignment);
		ov("memreq memoryTypeBits", vkmemreqs_ub_para.memoryTypeBits);

	VkMemoryAllocateInfo vkmemallo_info_para;
		vkmemallo_info_para.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkmemallo_info_para.pNext			= NULL;
		vkmemallo_info_para.allocationSize	= vkmemreqs_ub_para.size;
		vkmemallo_info_para.memoryTypeIndex	= mem_index_ub_para;

	VkDeviceMemory vkdevmem_ub_para;
	vr("vkAllocateMemory", &vkres, 
		vkAllocateMemory(vkld[0], &vkmemallo_info_para, NULL, &vkdevmem_ub_para) );

	vr("vkBindBufferMemory", &vkres, 
		vkBindBufferMemory(vkld[0], vkbuff_para, vkdevmem_ub_para, 0) );

	UniBuf_WF ub_wf;
		ub_wf.wsize = wsize_pack( window_size );
		ub_wf.frame = uint32_t(0);
		ub_wf.minfo = minfo_pack( mouse_info  );

	ov("UniBuf_WF Size", sizeof(ub_wf));

	void *pvoid_memmap_para;
	va("vkMapMemory", &vkres, pvoid_memmap_para,
		vkMapMemory(vkld[0], vkdevmem_ub_para, vkDescBuff_info_para.offset, vkDescBuff_info_para.range, 0, &pvoid_memmap_para) );

	rv("memcpy");
		memcpy(pvoid_memmap_para, &ub_wf, sizeof(ub_wf));

	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_ub_para[0], 0, NULL);
	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_ub_para[1], 0, NULL);

	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_para_sample_0[0], 0, NULL);
	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_para_sample_0[1], 0, NULL);

	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_para_sample_1[0], 0, NULL);
	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_para_sample_1[1], 0, NULL);

	VkPipelineLayoutCreateInfo vkpipelay_info_para;
		vkpipelay_info_para.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	nf(&vkpipelay_info_para);
		vkpipelay_info_para.setLayoutCount 				= 1;
		vkpipelay_info_para.pSetLayouts 				= &vkdescsetlay_para;
		vkpipelay_info_para.pushConstantRangeCount 		= 0;
		vkpipelay_info_para.pPushConstantRanges 		= NULL;

	VkPipelineLayout vkpipelay_para;
	va("vkCreatePipelineLayout", &vkres, vkpipelay_para,
		vkCreatePipelineLayout(vkld[0], &vkpipelay_info_para, NULL, &vkpipelay_para) );

	VkPipelineShaderStageCreateInfo vkgfxpipe_ss_info_para[2];
		vkgfxpipe_ss_info_para[0].sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	nf(&vkgfxpipe_ss_info_para[0]);
		vkgfxpipe_ss_info_para[0].stage					= VK_SHADER_STAGE_VERTEX_BIT;
		vkgfxpipe_ss_info_para[0].module				= vkshademod_vert[0];
		vkgfxpipe_ss_info_para[0].pName					= "main";
		vkgfxpipe_ss_info_para[0].pSpecializationInfo	= NULL;
		vkgfxpipe_ss_info_para[1].sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	nf(&vkgfxpipe_ss_info_para[1]);
		vkgfxpipe_ss_info_para[1].stage					= VK_SHADER_STAGE_FRAGMENT_BIT;
		vkgfxpipe_ss_info_para[1].module				= vkshademod_frag[1];
		vkgfxpipe_ss_info_para[1].pName					= "main";
		vkgfxpipe_ss_info_para[1].pSpecializationInfo	= NULL;

	VkGraphicsPipelineCreateInfo vkgfxpipe_info_para[2];
		vkgfxpipe_info_para[0].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	nf(&vkgfxpipe_info_para[0]);
		vkgfxpipe_info_para[0].stageCount 				= 2;
		vkgfxpipe_info_para[0].pStages 					= vkgfxpipe_ss_info_para;
		vkgfxpipe_info_para[0].pVertexInputState 		= &vkpipevertinput_info;
		vkgfxpipe_info_para[0].pInputAssemblyState 		= &vkpipeinputass_info;
		vkgfxpipe_info_para[0].pTessellationState 		= NULL;
		vkgfxpipe_info_para[0].pViewportState 			= &vkpipeviewport_info;
		vkgfxpipe_info_para[0].pRasterizationState 		= &vkpiperastinfo;
		vkgfxpipe_info_para[0].pMultisampleState 		= &vkpipems_info;
		vkgfxpipe_info_para[0].pDepthStencilState 		= NULL;
		vkgfxpipe_info_para[0].pColorBlendState 		= &vkpipecolblend;
		vkgfxpipe_info_para[0].pDynamicState 			= NULL;
		vkgfxpipe_info_para[0].layout 					= vkpipelay_para;
		vkgfxpipe_info_para[0].renderPass 				= vkrendpass_para[0];
		vkgfxpipe_info_para[0].subpass 					= 0;
		vkgfxpipe_info_para[0].basePipelineHandle 		= VK_NULL_HANDLE;
		vkgfxpipe_info_para[0].basePipelineIndex 		= -1;
		vkgfxpipe_info_para[1].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	nf(&vkgfxpipe_info_para[1]);
		vkgfxpipe_info_para[1].stageCount 				= 2;
		vkgfxpipe_info_para[1].pStages 					= vkgfxpipe_ss_info_para;
		vkgfxpipe_info_para[1].pVertexInputState 		= &vkpipevertinput_info;
		vkgfxpipe_info_para[1].pInputAssemblyState 		= &vkpipeinputass_info;
		vkgfxpipe_info_para[1].pTessellationState 		= NULL;
		vkgfxpipe_info_para[1].pViewportState 			= &vkpipeviewport_info;
		vkgfxpipe_info_para[1].pRasterizationState 		= &vkpiperastinfo;
		vkgfxpipe_info_para[1].pMultisampleState 		= &vkpipems_info;
		vkgfxpipe_info_para[1].pDepthStencilState 		= NULL;
		vkgfxpipe_info_para[1].pColorBlendState 		= &vkpipecolblend;
		vkgfxpipe_info_para[1].pDynamicState 			= NULL;
		vkgfxpipe_info_para[1].layout 					= vkpipelay_para;
		vkgfxpipe_info_para[1].renderPass 				= vkrendpass_para[1];
		vkgfxpipe_info_para[1].subpass 					= 0;
		vkgfxpipe_info_para[1].basePipelineHandle 		= VK_NULL_HANDLE;
		vkgfxpipe_info_para[1].basePipelineIndex 		= -1;

	VkPipeline vkpipe_para[2];
	va("vkCreateGraphicsPipelines", &vkres, vkpipe_para[0],
		vkCreateGraphicsPipelines(vkld[0], VK_NULL_HANDLE, 1, &vkgfxpipe_info_para[0], NULL, &vkpipe_para[0]) );
	va("vkCreateGraphicsPipelines", &vkres, vkpipe_para[1],
		vkCreateGraphicsPipelines(vkld[0], VK_NULL_HANDLE, 1, &vkgfxpipe_info_para[1], NULL, &vkpipe_para[1]) );

//	TODO	TODO	TODO	TODO


	std::string loadimg_name = "LOAD/IMG0.PAM";
	IMG_512_288 loadimg_data = PAM_to_IMG(loadimg(loadimg_name));

	VkDeviceSize vkdevsize_para_loadimg;
		vkdevsize_para_loadimg = sizeof(loadimg_data);
	ov("LoadIMG buffer size", vkdevsize_para_loadimg);

	VkBufferCreateInfo vkbuff_info_para_loadimg;
		vkbuff_info_para_loadimg.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	nf(&vkbuff_info_para_loadimg);
		vkbuff_info_para_loadimg.size 						= vkdevsize_para_loadimg;
		vkbuff_info_para_loadimg.usage 						= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		vkbuff_info_para_loadimg.sharingMode 				= VK_SHARING_MODE_EXCLUSIVE;
		vkbuff_info_para_loadimg.queueFamilyIndexCount 		= 1;
		vkbuff_info_para_loadimg.pQueueFamilyIndices 		= &GQF_IDX;

	VkBuffer vkbuff_para_loadimg;
	va("vkCreateBuffer", &vkres, vkbuff_para_loadimg,
		vkCreateBuffer(vkld[0], &vkbuff_info_para_loadimg, NULL, &vkbuff_para_loadimg) );

	int mem_index_para_loadimg = UINT32_MAX;
	for(int i = 0; i < vkpd_memprops.memoryTypeCount; i++) {
		if( vkpd_memprops.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		&&	mem_index_para_loadimg == UINT32_MAX ) {
			mem_index_para_loadimg = i; } }
	ov("mem_index_para_loadimg", mem_index_para_loadimg);

	VkMemoryRequirements vkmemreqs_para_loadimg;
	rv("vkGetBufferMemoryRequirements");
		vkGetBufferMemoryRequirements(vkld[0], vkbuff_para_loadimg, &vkmemreqs_para_loadimg);
		ov("memreq size", 			vkmemreqs_para_loadimg.size);
		ov("memreq alignment", 		vkmemreqs_para_loadimg.alignment);
		ov("memreq memoryTypeBits", vkmemreqs_para_loadimg.memoryTypeBits);

	VkMemoryAllocateInfo vkmemallo_info_para_loadimg;
		vkmemallo_info_para_loadimg.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkmemallo_info_para_loadimg.pNext			= NULL;
		vkmemallo_info_para_loadimg.allocationSize	= vkmemreqs_para_loadimg.size;
		vkmemallo_info_para_loadimg.memoryTypeIndex	= mem_index_para_loadimg;

	VkDeviceMemory vkdevmem_para_loadimg;
	vr("vkAllocateMemory", &vkres, 
		vkAllocateMemory(vkld[0], &vkmemallo_info_para_loadimg, NULL, &vkdevmem_para_loadimg) );

	vr("vkBindBufferMemory", &vkres, 
		vkBindBufferMemory(vkld[0], vkbuff_para_loadimg, vkdevmem_para_loadimg, 0) );

	void *pvoid_memmap_para_loadimg;
	va("vkMapMemory", &vkres, pvoid_memmap_para_loadimg,
		vkMapMemory(vkld[0], vkdevmem_para_loadimg, 0, VK_WHOLE_SIZE, 0, &pvoid_memmap_para_loadimg) );

	rv("memcpy");
		memcpy(pvoid_memmap_para_loadimg, &loadimg_data, sizeof(loadimg_data));

	VkImageMemoryBarrier vkimgmembar_para_dst2sro[2];
		vkimgmembar_para_dst2sro[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_para_dst2sro[0].pNext 					= NULL;
		vkimgmembar_para_dst2sro[0].srcAccessMask 			= 0;
		vkimgmembar_para_dst2sro[0].dstAccessMask 			= 0;
		vkimgmembar_para_dst2sro[0].oldLayout 				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkimgmembar_para_dst2sro[0].newLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_para_dst2sro[0].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_dst2sro[0].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_dst2sro[0].image 					= vkimg_para[0];
		vkimgmembar_para_dst2sro[0].subresourceRange 		= vkimgsubrange;
		vkimgmembar_para_dst2sro[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_para_dst2sro[1].pNext 					= NULL;
		vkimgmembar_para_dst2sro[1].srcAccessMask 			= 0;
		vkimgmembar_para_dst2sro[1].dstAccessMask 			= 0;
		vkimgmembar_para_dst2sro[1].oldLayout 				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkimgmembar_para_dst2sro[1].newLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_para_dst2sro[1].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_dst2sro[1].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_dst2sro[1].image 					= vkimg_para[1];
		vkimgmembar_para_dst2sro[1].subresourceRange 		= vkimgsubrange;

	VkImageMemoryBarrier vkimgmembar_para_sro2dst[2];
		vkimgmembar_para_sro2dst[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_para_sro2dst[0].pNext 					= NULL;
		vkimgmembar_para_sro2dst[0].srcAccessMask 			= 0;
		vkimgmembar_para_sro2dst[0].dstAccessMask 			= 0;
		vkimgmembar_para_sro2dst[0].oldLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_para_sro2dst[0].newLayout 				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkimgmembar_para_sro2dst[0].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_sro2dst[0].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_sro2dst[0].image 					= vkimg_para[0];
		vkimgmembar_para_sro2dst[0].subresourceRange 		= vkimgsubrange;
		vkimgmembar_para_sro2dst[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_para_sro2dst[1].pNext 					= NULL;
		vkimgmembar_para_sro2dst[1].srcAccessMask 			= 0;
		vkimgmembar_para_sro2dst[1].dstAccessMask 			= 0;
		vkimgmembar_para_sro2dst[1].oldLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_para_sro2dst[1].newLayout 				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkimgmembar_para_sro2dst[1].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_sro2dst[1].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_para_sro2dst[1].image 					= vkimg_para[1];
		vkimgmembar_para_sro2dst[1].subresourceRange 		= vkimgsubrange;

/**/


//	Fragment Shader
	for(int i = 0; i < 2; i++) {
		vr("vkBeginCommandBuffer", &vkres, 
			vkBeginCommandBuffer(vkcombuf_para[i], &vkcombufbegin_info[i]) );

			rv("vkCmdBeginRenderPass");
				vkCmdBeginRenderPass (
					vkcombuf_para[i], &vkrpbegininfo_para[i], VK_SUBPASS_CONTENTS_INLINE );

				rv("vkCmdBindPipeline");
					vkCmdBindPipeline (
						vkcombuf_para[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipe_para[i] );

				rv("vkCmdBindDescriptorSets");
					vkCmdBindDescriptorSets (
						vkcombuf_para[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipelay_para,
						0, 1, &vkdescset_para[i], 0, NULL );

				rv("vkCmdDraw");
					vkCmdDraw (
						vkcombuf_para[i], 3, 1, 0, 0 );

			rv("vkCmdEndRenderPass");
				vkCmdEndRenderPass(vkcombuf_para[i]);

		vr("vkEndCommandBuffer", &vkres, 
			vkEndCommandBuffer(vkcombuf_para[i]) ); }

//	Image Load
	for(int i = 0; i < 2; i++) {
		vr("vkBeginCommandBuffer", &vkres, 
			vkBeginCommandBuffer(vkcombuf_para_loadimg[i], &vkcombufbegin_info[i]) );

/*			rv("vkCmdPipelineBarrier");
				vkCmdPipelineBarrier (
					vkcombuf_para_loadimg[i],
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT,
					0, NULL, 0, NULL,
					1, &vkimgmembar_para_sro2dst[i] );

			rv("vkCmdCopyBufferToImage");
				vkCmdCopyBufferToImage (
					vkcombuf_para_loadimg[i], vkbuff_para_loadimg, vkimg_para[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &vkbuffimgcopy );

			rv("vkCmdPipelineBarrier");
				vkCmdPipelineBarrier (
					vkcombuf_para_loadimg[i],
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT,
					0, NULL, 0, NULL,
					1, &vkimgmembar_para_dst2sro[i] );*/

		vr("vkEndCommandBuffer", &vkres, 
			vkEndCommandBuffer(vkcombuf_para_loadimg[i]) ); }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "SUBMIT PARA_LOOP");		/**/
	///////////////////////////////////////////////////

/*	for(int i = 0; i < 2; i++) {
		if(valid) {
			rv("vkcombuf_para");
			VkSubmitInfo vksub_info[1];
				vksub_info[0].sType	= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				vksub_info[0].pNext					= NULL;
				vksub_info[0].waitSemaphoreCount	= 0;
				vksub_info[0].pWaitSemaphores		= NULL;
				vksub_info[0].pWaitDstStageMask		= NULL;
				vksub_info[0].commandBufferCount	= 1;
				vksub_info[0].pCommandBuffers		= &vkcombuf_para[i];
				vksub_info[0].signalSemaphoreCount	= 0;
				vksub_info[0].pSignalSemaphores		= NULL;
			vr("vkQueueSubmit", &vkres, 
				vkQueueSubmit(vkq[0], 1, vksub_info, VK_NULL_HANDLE) ); } }



	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "RECORD WORK_LOOP");		/**/
	///////////////////////////////////////////////////

	VkAttachmentDescription vkattdesc_work[2];
		vkattdesc_work[0].flags 			= 0;
		vkattdesc_work[0].format 			= vkimg_info_work.format;
		vkattdesc_work[0].samples 			= VK_SAMPLE_COUNT_1_BIT;
		vkattdesc_work[0].loadOp 			= VK_ATTACHMENT_LOAD_OP_LOAD;
		vkattdesc_work[0].storeOp 			= VK_ATTACHMENT_STORE_OP_STORE;
		vkattdesc_work[0].stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkattdesc_work[0].stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkattdesc_work[0].initialLayout 	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkattdesc_work[0].finalLayout 		= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		vkattdesc_work[1].flags 			= 0;
		vkattdesc_work[1].format 			= vkimg_info_work.format;
		vkattdesc_work[1].samples 			= VK_SAMPLE_COUNT_1_BIT;
		vkattdesc_work[1].loadOp 			= VK_ATTACHMENT_LOAD_OP_LOAD;
		vkattdesc_work[1].storeOp 			= VK_ATTACHMENT_STORE_OP_STORE;
		vkattdesc_work[1].stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkattdesc_work[1].stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkattdesc_work[1].initialLayout 	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkattdesc_work[1].finalLayout 		= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	VkAttachmentReference vkattref_work;
		vkattref_work.attachment 	= 0;
		vkattref_work.layout 		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription vksubpassdesc_work;
		vksubpassdesc_work.flags 						= 0;
		vksubpassdesc_work.pipelineBindPoint 			= VK_PIPELINE_BIND_POINT_GRAPHICS;
		vksubpassdesc_work.inputAttachmentCount 		= 0;
		vksubpassdesc_work.pInputAttachments 			= NULL;
		vksubpassdesc_work.colorAttachmentCount 		= 1;
		vksubpassdesc_work.pColorAttachments 			= &vkattref_work;
		vksubpassdesc_work.pResolveAttachments 			= NULL;
		vksubpassdesc_work.pDepthStencilAttachment 		= NULL;
		vksubpassdesc_work.preserveAttachmentCount 		= 0;
		vksubpassdesc_work.pPreserveAttachments 		= NULL;

	VkRenderPassCreateInfo vkrendpass_info_work[2];
		vkrendpass_info_work[0].sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	nf(&vkrendpass_info_work[0]);
		vkrendpass_info_work[0].attachmentCount 	= 1;
		vkrendpass_info_work[0].pAttachments 		= &vkattdesc_work[0];
		vkrendpass_info_work[0].subpassCount 		= 1;
		vkrendpass_info_work[0].pSubpasses 			= &vksubpassdesc_work;
		vkrendpass_info_work[0].dependencyCount 	= 0;
		vkrendpass_info_work[0].pDependencies 		= NULL;
		vkrendpass_info_work[1].sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	nf(&vkrendpass_info_work[1]);
		vkrendpass_info_work[1].attachmentCount 	= 1;
		vkrendpass_info_work[1].pAttachments 		= &vkattdesc_work[1];
		vkrendpass_info_work[1].subpassCount 		= 1;
		vkrendpass_info_work[1].pSubpasses 			= &vksubpassdesc_work;
		vkrendpass_info_work[1].dependencyCount 	= 0;
		vkrendpass_info_work[1].pDependencies 		= NULL;

	VkRenderPass vkrendpass_work[2];
	va("vkCreateRenderPass", &vkres, vkrendpass_work[0],
		vkCreateRenderPass(vkld[0], &vkrendpass_info_work[0], NULL, &vkrendpass_work[0]) );
	va("vkCreateRenderPass", &vkres, vkrendpass_work[1],
		vkCreateRenderPass(vkld[0], &vkrendpass_info_work[1], NULL, &vkrendpass_work[1]) );

	VkFramebufferCreateInfo vkframebuff_info_work[2];
		vkframebuff_info_work[0].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	nf(&vkframebuff_info_work[0]);
		vkframebuff_info_work[0].renderPass 		= vkrendpass_work[0];
		vkframebuff_info_work[0].attachmentCount 	= 1;
		vkframebuff_info_work[0].pAttachments 		= &vkimgview_work[0];
		vkframebuff_info_work[0].width 				= vksurf_ables[0].currentExtent.width;
		vkframebuff_info_work[0].height 			= vksurf_ables[0].currentExtent.height;
		vkframebuff_info_work[0].layers 			= 1;
		vkframebuff_info_work[1].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	nf(&vkframebuff_info_work[1]);
		vkframebuff_info_work[1].renderPass 		= vkrendpass_work[1];
		vkframebuff_info_work[1].attachmentCount 	= 1;
		vkframebuff_info_work[1].pAttachments 		= &vkimgview_work[1];
		vkframebuff_info_work[1].width 				= vksurf_ables[0].currentExtent.width;
		vkframebuff_info_work[1].height 			= vksurf_ables[0].currentExtent.height;
		vkframebuff_info_work[1].layers 			= 1;

	VkFramebuffer vkframebuff_work[2];
	va("vkCreateFramebuffer", &vkres, vkframebuff_work[0],
		vkCreateFramebuffer(vkld[0], &vkframebuff_info_work[0], NULL, &vkframebuff_work[0]) );
	va("vkCreateFramebuffer", &vkres, vkframebuff_work[1],
		vkCreateFramebuffer(vkld[0], &vkframebuff_info_work[1], NULL, &vkframebuff_work[1]) );

	VkRenderPassBeginInfo vkrpbegininfo_work[2];
		vkrpbegininfo_work[0].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkrpbegininfo_work[0].pNext 				= NULL;
		vkrpbegininfo_work[0].renderPass 			= vkrendpass_work[0];
		vkrpbegininfo_work[0].framebuffer 			= vkframebuff_work[0];
		vkrpbegininfo_work[0].renderArea 			= vkrect2d;
		vkrpbegininfo_work[0].clearValueCount 		= 1;
		vkrpbegininfo_work[0].pClearValues 			= &vkclearval[2];
		vkrpbegininfo_work[1].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkrpbegininfo_work[1].pNext 				= NULL;
		vkrpbegininfo_work[1].renderPass 			= vkrendpass_work[1];
		vkrpbegininfo_work[1].framebuffer 			= vkframebuff_work[1];
		vkrpbegininfo_work[1].renderArea 			= vkrect2d;
		vkrpbegininfo_work[1].clearValueCount 		= 1;
		vkrpbegininfo_work[1].pClearValues 			= &vkclearval[3];

	const int WORKIMGS = 2;

	VkDescriptorSetLayoutBinding vkdescsetlaybind_work[3];
		vkdescsetlaybind_work[0].binding				= 1;
		vkdescsetlaybind_work[0].descriptorType			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		vkdescsetlaybind_work[0].descriptorCount		= 1;
		vkdescsetlaybind_work[0].stageFlags				= VK_SHADER_STAGE_FRAGMENT_BIT;
		vkdescsetlaybind_work[0].pImmutableSamplers		= NULL;
		vkdescsetlaybind_work[1].binding				= 0;
		vkdescsetlaybind_work[1].descriptorType			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vkdescsetlaybind_work[1].descriptorCount		= 1;
		vkdescsetlaybind_work[1].stageFlags				= VK_SHADER_STAGE_FRAGMENT_BIT;
		vkdescsetlaybind_work[1].pImmutableSamplers		= NULL;
		vkdescsetlaybind_work[2].binding				= 2;
		vkdescsetlaybind_work[2].descriptorType			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		vkdescsetlaybind_work[2].descriptorCount		= 1;
		vkdescsetlaybind_work[2].stageFlags				= VK_SHADER_STAGE_FRAGMENT_BIT;
		vkdescsetlaybind_work[2].pImmutableSamplers		= NULL;

	VkDescriptorSetLayoutCreateInfo vkdescset_info_work;
		vkdescset_info_work.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	nf(&vkdescset_info_work);
		vkdescset_info_work.bindingCount	= 3;
		vkdescset_info_work.pBindings		= vkdescsetlaybind_work;

	VkDescriptorSetLayout vkdescsetlay_work;
	va("vkCreateDescriptorSetLayout", &vkres, vkdescsetlay_work,
		vkCreateDescriptorSetLayout(vkld[0], &vkdescset_info_work, NULL, &vkdescsetlay_work) );

	VkDescriptorPoolSize vkdescpoolsize_work[3];
		vkdescpoolsize_work[0].type 				= vkdescsetlaybind_work[0].descriptorType;
		vkdescpoolsize_work[0].descriptorCount 		= 3;
		vkdescpoolsize_work[1].type 				= vkdescsetlaybind_work[1].descriptorType;
		vkdescpoolsize_work[1].descriptorCount 		= 7;
		vkdescpoolsize_work[2].type 				= vkdescsetlaybind_work[2].descriptorType;
		vkdescpoolsize_work[2].descriptorCount 		= 1;

	VkDescriptorPoolCreateInfo vkdescpool_info_work[2];
		vkdescpool_info_work[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	nf(&vkdescpool_info_work[0]);
		vkdescpool_info_work[0].maxSets 			= WORKIMGS;
		vkdescpool_info_work[0].poolSizeCount 	= 1;
		vkdescpool_info_work[0].pPoolSizes 		= &vkdescpoolsize_work[0];
		vkdescpool_info_work[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	nf(&vkdescpool_info_work[1]);
		vkdescpool_info_work[1].maxSets 			= WORKIMGS;
		vkdescpool_info_work[1].poolSizeCount 	= 1;
		vkdescpool_info_work[1].pPoolSizes 		= &vkdescpoolsize_work[1];

	VkDescriptorPool vkdescpool_work[2];
	va("vkCreateDescriptorPool", &vkres, vkdescpool_work[0],
		vkCreateDescriptorPool(vkld[0], &vkdescpool_info_work[0], NULL, &vkdescpool_work[0]) );
	va("vkCreateDescriptorPool", &vkres, vkdescpool_work[1],
		vkCreateDescriptorPool(vkld[0], &vkdescpool_info_work[1], NULL, &vkdescpool_work[1]) );

	VkDescriptorSetAllocateInfo vkdescsetallo_info_work[2];
		vkdescsetallo_info_work[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		vkdescsetallo_info_work[0].pNext 				= NULL;
		vkdescsetallo_info_work[0].descriptorPool 		= vkdescpool_work[0];
		vkdescsetallo_info_work[0].descriptorSetCount 	= 1;
		vkdescsetallo_info_work[0].pSetLayouts 			= &vkdescsetlay_work;
		vkdescsetallo_info_work[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		vkdescsetallo_info_work[1].pNext 				= NULL;
		vkdescsetallo_info_work[1].descriptorPool 		= vkdescpool_work[1];
		vkdescsetallo_info_work[1].descriptorSetCount 	= 1;
		vkdescsetallo_info_work[1].pSetLayouts 			= &vkdescsetlay_work;

	VkDescriptorSet vkdescset_work[2];
	va("vkAllocateDescriptorSets", &vkres, vkdescset_work[0],
		vkAllocateDescriptorSets(vkld[0], &vkdescsetallo_info_work[0], &vkdescset_work[0]) );
	va("vkAllocateDescriptorSets", &vkres, vkdescset_work[1],
		vkAllocateDescriptorSets(vkld[0], &vkdescsetallo_info_work[1], &vkdescset_work[1]) );

	VkDescriptorImageInfo vkdescimg_info[2];
		vkdescimg_info[0].sampler			= vksampler;
		vkdescimg_info[0].imageView			= vkimgview_work[1]; // Reference the other image
		vkdescimg_info[0].imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkdescimg_info[1].sampler			= vksampler;
		vkdescimg_info[1].imageView			= vkimgview_work[0]; // Reference the other image
		vkdescimg_info[1].imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo vkdescimg_info_sample_1[2];
		vkdescimg_info_sample_1[0].sampler			= vksampler;
		vkdescimg_info_sample_1[0].imageView		= vkimgview_para[0]; // 1 Reference the other image
		vkdescimg_info_sample_1[0].imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkdescimg_info_sample_1[1].sampler			= vksampler;
		vkdescimg_info_sample_1[1].imageView		= vkimgview_para[0]; // 0 Reference the other image
		vkdescimg_info_sample_1[1].imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet vkwritedescset[2];
		vkwritedescset[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset[0].pNext 				= NULL;
		vkwritedescset[0].dstSet 				= vkdescset_work[0];
		vkwritedescset[0].dstBinding 			= 1;
		vkwritedescset[0].dstArrayElement 		= 0;
		vkwritedescset[0].descriptorCount 		= 1;
		vkwritedescset[0].descriptorType 		= vkdescsetlaybind_work[0].descriptorType;
		vkwritedescset[0].pImageInfo 			= &vkdescimg_info[0];
		vkwritedescset[0].pBufferInfo 			= NULL;
		vkwritedescset[0].pTexelBufferView 		= NULL;
		vkwritedescset[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset[1].pNext 				= NULL;
		vkwritedescset[1].dstSet 				= vkdescset_work[1];
		vkwritedescset[1].dstBinding 			= 1;
		vkwritedescset[1].dstArrayElement 		= 0;
		vkwritedescset[1].descriptorCount 		= 1;
		vkwritedescset[1].descriptorType 		= vkdescsetlaybind_work[0].descriptorType;
		vkwritedescset[1].pImageInfo 			= &vkdescimg_info[1];
		vkwritedescset[1].pBufferInfo 			= NULL;
		vkwritedescset[1].pTexelBufferView 		= NULL;

	VkWriteDescriptorSet vkwritedescset_work_sample_1[2];
		vkwritedescset_work_sample_1[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_work_sample_1[0].pNext 				= NULL;
		vkwritedescset_work_sample_1[0].dstSet 				= vkdescset_work[0];
		vkwritedescset_work_sample_1[0].dstBinding 			= 2;
		vkwritedescset_work_sample_1[0].dstArrayElement 	= 0;
		vkwritedescset_work_sample_1[0].descriptorCount 	= 1;
		vkwritedescset_work_sample_1[0].descriptorType 		= vkdescsetlaybind_work[2].descriptorType;
		vkwritedescset_work_sample_1[0].pImageInfo 			= &vkdescimg_info_sample_1[0];
		vkwritedescset_work_sample_1[0].pBufferInfo 		= NULL;
		vkwritedescset_work_sample_1[0].pTexelBufferView 	= NULL;
		vkwritedescset_work_sample_1[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_work_sample_1[1].pNext 				= NULL;
		vkwritedescset_work_sample_1[1].dstSet 				= vkdescset_work[1];
		vkwritedescset_work_sample_1[1].dstBinding 			= 2;
		vkwritedescset_work_sample_1[1].dstArrayElement 	= 0;
		vkwritedescset_work_sample_1[1].descriptorCount 	= 1;
		vkwritedescset_work_sample_1[1].descriptorType 		= vkdescsetlaybind_work[2].descriptorType;
		vkwritedescset_work_sample_1[1].pImageInfo 			= &vkdescimg_info_sample_1[1];
		vkwritedescset_work_sample_1[1].pBufferInfo 		= NULL;
		vkwritedescset_work_sample_1[1].pTexelBufferView 	= NULL;

	VkDeviceSize vkdevsize_work;
		vkdevsize_work = sizeof(UniBuf);
	ov("UniBuf size", vkdevsize_work);

	VkBufferCreateInfo vkbuff_info_work;
		vkbuff_info_work.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	nf(&vkbuff_info_work);
		vkbuff_info_work.size 						= vkdevsize_work;
		vkbuff_info_work.usage 						= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		vkbuff_info_work.sharingMode 				= VK_SHARING_MODE_EXCLUSIVE;
		vkbuff_info_work.queueFamilyIndexCount 		= 1;
		vkbuff_info_work.pQueueFamilyIndices 		= &GQF_IDX;

	VkBuffer vkbuff_work;
	va("vkCreateBuffer", &vkres, vkbuff_work,
		vkCreateBuffer(vkld[0], &vkbuff_info_work, NULL, &vkbuff_work) );

	VkDescriptorBufferInfo vkDescBuff_info_work;
		vkDescBuff_info_work.buffer 		= vkbuff_work;
		vkDescBuff_info_work.offset 		= 0;
		vkDescBuff_info_work.range 		= VK_WHOLE_SIZE;

	VkWriteDescriptorSet vkwritedescset_ub_work[2];
		vkwritedescset_ub_work[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_ub_work[0].pNext 				= NULL;
		vkwritedescset_ub_work[0].dstSet 				= vkdescset_work[0];
		vkwritedescset_ub_work[0].dstBinding 			= 0;
		vkwritedescset_ub_work[0].dstArrayElement 		= 0;
		vkwritedescset_ub_work[0].descriptorCount 		= 1;
		vkwritedescset_ub_work[0].descriptorType 		= vkdescsetlaybind_work[1].descriptorType;
		vkwritedescset_ub_work[0].pImageInfo 			= NULL;
		vkwritedescset_ub_work[0].pBufferInfo 			= &vkDescBuff_info_work;
		vkwritedescset_ub_work[0].pTexelBufferView 		= NULL;
		vkwritedescset_ub_work[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkwritedescset_ub_work[1].pNext 				= NULL;
		vkwritedescset_ub_work[1].dstSet 				= vkdescset_work[1];
		vkwritedescset_ub_work[1].dstBinding 			= 0;
		vkwritedescset_ub_work[1].dstArrayElement 		= 0;
		vkwritedescset_ub_work[1].descriptorCount 		= 1;
		vkwritedescset_ub_work[1].descriptorType 		= vkdescsetlaybind_work[1].descriptorType;
		vkwritedescset_ub_work[1].pImageInfo 			= NULL;
		vkwritedescset_ub_work[1].pBufferInfo 			= &vkDescBuff_info_work;
		vkwritedescset_ub_work[1].pTexelBufferView 		= NULL;

	int mem_index_ub_work = UINT32_MAX;
	for(int i = 0; i < vkpd_memprops.memoryTypeCount; i++) {
		if( vkpd_memprops.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		&&	mem_index_ub_work == UINT32_MAX ) {
			mem_index_ub_work = i; } }
	ov("mem_index_ub_work", mem_index_ub_work);

	VkMemoryRequirements vkmemreqs_ub_work;
	rv("vkGetBufferMemoryRequirements");
		vkGetBufferMemoryRequirements(vkld[0], vkbuff_work, &vkmemreqs_ub_work);
		ov("memreq size", 			vkmemreqs_ub_work.size);
		ov("memreq alignment", 		vkmemreqs_ub_work.alignment);
		ov("memreq memoryTypeBits", vkmemreqs_ub_work.memoryTypeBits);

	VkMemoryAllocateInfo vkmemallo_info_work;
		vkmemallo_info_work.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkmemallo_info_work.pNext			= NULL;
		vkmemallo_info_work.allocationSize	= vkmemreqs_ub_work.size;
		vkmemallo_info_work.memoryTypeIndex	= mem_index_ub_work;

	VkDeviceMemory vkdevmem_ub_work;
	vr("vkAllocateMemory", &vkres, 
		vkAllocateMemory(vkld[0], &vkmemallo_info_work, NULL, &vkdevmem_ub_work) );

	vr("vkBindBufferMemory", &vkres, 
		vkBindBufferMemory(vkld[0], vkbuff_work, vkdevmem_ub_work, 0) );

	UniBuf ub;
		ub.wsize = wsize_pack( window_size );
		ub.frame = uint32_t(0);
		ub.minfo = minfo_pack( mouse_info  );
		ub.i0  = 0; ub.i1  = 0; ub.i2  = 0; ub.i3  = 0;
		ub.v0  = 0; ub.v1  = 0; ub.v2  = 0; ub.v3  = 0;
		ub.v4  = 0; ub.v5  = 0; ub.v6  = 0; ub.v7  = 0;
		ub.v8  = 0; ub.v9  = 0; ub.v10 = 0; ub.v11 = 0;
		ub.v12 = 0; ub.v13 = 0; ub.v14 = 0; ub.v15 = 0;
		ub.v16 = 0; ub.v17 = 0; ub.v18 = 0; ub.v19 = 0;
		ub.v20 = 0; ub.v21 = 0; ub.v22 = 0; ub.v23 = 0;
		ub.v24 = 0; ub.v25 = 0; ub.v26 = 0; ub.v27 = 0;
		ub.v28 = 0; ub.v29 = 0; ub.v30 = 0; ub.v31 = 0;
		ub.v32 = 0; ub.v33 = 0; ub.v34 = 0; ub.v35 = 0;
		ub.v36 = 0; ub.v37 = 0; ub.v38 = 0; ub.v39 = 0;
		ub.v40 = 0; ub.v41 = 0; ub.v42 = 0; ub.v43 = 0;
		ub.v44 = 0; ub.v45 = 0; ub.v46 = 0; ub.v47 = 0;
		ub.scale = float(96.0);
		ub.zoom  = float(0.0);

	ov("UniBuf Size", sizeof(ub));

	void *pvoid_memmap_work;
	va("vkMapMemory", &vkres, pvoid_memmap_work,
		vkMapMemory(vkld[0], vkdevmem_ub_work, vkDescBuff_info_work.offset, vkDescBuff_info_work.range, 0, &pvoid_memmap_work) );

	rv("memcpy");
		memcpy(pvoid_memmap_work, &ub, sizeof(ub));

	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset[0], 0, NULL);
	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset[1], 0, NULL);

	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_ub_work[0], 0, NULL);
	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_ub_work[1], 0, NULL);

	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_work_sample_1[0], 0, NULL);
	rv("vkUpdateDescriptorSets");
		vkUpdateDescriptorSets(vkld[0], 1, &vkwritedescset_work_sample_1[1], 0, NULL);

	VkPipelineLayoutCreateInfo vkpipelay_info_work;
		vkpipelay_info_work.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	nf(&vkpipelay_info_work);
		vkpipelay_info_work.setLayoutCount 				= 1;
		vkpipelay_info_work.pSetLayouts 				= &vkdescsetlay_work;
		vkpipelay_info_work.pushConstantRangeCount 		= 0;
		vkpipelay_info_work.pPushConstantRanges 		= NULL;

	VkPipelineLayout vkpipelay_work;
	va("vkCreatePipelineLayout", &vkres, vkpipelay_work,
		vkCreatePipelineLayout(vkld[0], &vkpipelay_info_work, NULL, &vkpipelay_work) );

	VkGraphicsPipelineCreateInfo vkgfxpipe_info_work[2];
		vkgfxpipe_info_work[0].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	nf(&vkgfxpipe_info_work[0]);
		vkgfxpipe_info_work[0].stageCount 				= 2;
		vkgfxpipe_info_work[0].pStages 					= vkgfxpipe_ss_info;
		vkgfxpipe_info_work[0].pVertexInputState 		= &vkpipevertinput_info;
		vkgfxpipe_info_work[0].pInputAssemblyState 		= &vkpipeinputass_info;
		vkgfxpipe_info_work[0].pTessellationState 		= NULL;
		vkgfxpipe_info_work[0].pViewportState 			= &vkpipeviewport_info;
		vkgfxpipe_info_work[0].pRasterizationState 		= &vkpiperastinfo;
		vkgfxpipe_info_work[0].pMultisampleState 		= &vkpipems_info;
		vkgfxpipe_info_work[0].pDepthStencilState 		= NULL;
		vkgfxpipe_info_work[0].pColorBlendState 		= &vkpipecolblend;
		vkgfxpipe_info_work[0].pDynamicState 			= NULL;
		vkgfxpipe_info_work[0].layout 					= vkpipelay_work;
		vkgfxpipe_info_work[0].renderPass 				= vkrendpass_work[0];
		vkgfxpipe_info_work[0].subpass 					= 0;
		vkgfxpipe_info_work[0].basePipelineHandle 		= VK_NULL_HANDLE;
		vkgfxpipe_info_work[0].basePipelineIndex 		= -1;
		vkgfxpipe_info_work[1].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	nf(&vkgfxpipe_info_work[1]);
		vkgfxpipe_info_work[1].stageCount 				= 2;
		vkgfxpipe_info_work[1].pStages 					= vkgfxpipe_ss_info;
		vkgfxpipe_info_work[1].pVertexInputState 		= &vkpipevertinput_info;
		vkgfxpipe_info_work[1].pInputAssemblyState 		= &vkpipeinputass_info;
		vkgfxpipe_info_work[1].pTessellationState 		= NULL;
		vkgfxpipe_info_work[1].pViewportState 			= &vkpipeviewport_info;
		vkgfxpipe_info_work[1].pRasterizationState 		= &vkpiperastinfo;
		vkgfxpipe_info_work[1].pMultisampleState 		= &vkpipems_info;
		vkgfxpipe_info_work[1].pDepthStencilState 		= NULL;
		vkgfxpipe_info_work[1].pColorBlendState 		= &vkpipecolblend;
		vkgfxpipe_info_work[1].pDynamicState 			= NULL;
		vkgfxpipe_info_work[1].layout 					= vkpipelay_work;
		vkgfxpipe_info_work[1].renderPass 				= vkrendpass_work[1];
		vkgfxpipe_info_work[1].subpass 					= 0;
		vkgfxpipe_info_work[1].basePipelineHandle 		= VK_NULL_HANDLE;
		vkgfxpipe_info_work[1].basePipelineIndex 		= -1;

	VkPipeline vkpipe_work[2];
	va("vkCreateGraphicsPipelines", &vkres, vkpipe_work[0],
		vkCreateGraphicsPipelines(vkld[0], VK_NULL_HANDLE, 1, &vkgfxpipe_info_work[0], NULL, &vkpipe_work[0]) );
	va("vkCreateGraphicsPipelines", &vkres, vkpipe_work[1],
		vkCreateGraphicsPipelines(vkld[0], VK_NULL_HANDLE, 1, &vkgfxpipe_info_work[1], NULL, &vkpipe_work[1]) );

	for(int i = 0; i < 2; i++) {
		vr("vkBeginCommandBuffer", &vkres, 
			vkBeginCommandBuffer(vkcombuf_work[i], &vkcombufbegin_info[i]) );

			rv("vkCmdBeginRenderPass");
				vkCmdBeginRenderPass (
					vkcombuf_work[i], &vkrpbegininfo_work[i], VK_SUBPASS_CONTENTS_INLINE );

				rv("vkCmdBindPipeline");
					vkCmdBindPipeline (
						vkcombuf_work[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipe_work[i] );

				rv("vkCmdBindDescriptorSets");
					vkCmdBindDescriptorSets (
						vkcombuf_work[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipelay_work,
						0, 1, &vkdescset_work[i], 0, NULL );

				rv("vkCmdDraw");
					vkCmdDraw (
						vkcombuf_work[i], 3, 1, 0, 0 );

			rv("vkCmdEndRenderPass");
				vkCmdEndRenderPass(vkcombuf_work[i]);

		vr("vkEndCommandBuffer", &vkres, 
			vkEndCommandBuffer(vkcombuf_work[i]) ); }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "RECORD PRES_INIT");		/**/
	///////////////////////////////////////////////////

	VkImageViewCreateInfo vkimgview_info[2];
		vkimgview_info[0].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	nf(&vkimgview_info[0]);
		vkimgview_info[0].image 				= vkswap_img[0];
		vkimgview_info[0].viewType 				= VK_IMAGE_VIEW_TYPE_2D;
		vkimgview_info[0].format 				= vksurf_fmt[SURF_FMT].format;
		vkimgview_info[0].components.r			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info[0].components.g			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info[0].components.b			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info[0].components.a			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info[0].subresourceRange 		= vkimgsubrange;
		vkimgview_info[1].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	nf(&vkimgview_info[1]);
		vkimgview_info[1].image 				= vkswap_img[1];
		vkimgview_info[1].viewType 				= VK_IMAGE_VIEW_TYPE_2D;
		vkimgview_info[1].format 				= vksurf_fmt[SURF_FMT].format;
		vkimgview_info[1].components.r			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info[1].components.g			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info[1].components.b			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info[1].components.a			= VK_COMPONENT_SWIZZLE_IDENTITY;
		vkimgview_info[1].subresourceRange 		= vkimgsubrange;

	VkImageView vkimgview[2];
	va("vkCreateImageView", &vkres, vkimgview[0],
		vkCreateImageView(vkld[0], &vkimgview_info[0], NULL, &vkimgview[0]) );
	va("vkCreateImageView", &vkres, vkimgview[1],
		vkCreateImageView(vkld[0], &vkimgview_info[1], NULL, &vkimgview[1]) );

	VkImageMemoryBarrier vkimgmembar_pres_und2prs[2];
		vkimgmembar_pres_und2prs[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_pres_und2prs[0].pNext 					= NULL;
		vkimgmembar_pres_und2prs[0].srcAccessMask 			= 0;
		vkimgmembar_pres_und2prs[0].dstAccessMask 			= 0;
		vkimgmembar_pres_und2prs[0].oldLayout 				= VK_IMAGE_LAYOUT_UNDEFINED;
		vkimgmembar_pres_und2prs[0].newLayout 				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		vkimgmembar_pres_und2prs[0].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_und2prs[0].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_und2prs[0].image 					= vkswap_img[0];
		vkimgmembar_pres_und2prs[0].subresourceRange 		= vkimgsubrange;
		vkimgmembar_pres_und2prs[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_pres_und2prs[1].pNext 					= NULL;
		vkimgmembar_pres_und2prs[1].srcAccessMask 			= 0;
		vkimgmembar_pres_und2prs[1].dstAccessMask 			= 0;
		vkimgmembar_pres_und2prs[1].oldLayout 				= VK_IMAGE_LAYOUT_UNDEFINED;
		vkimgmembar_pres_und2prs[1].newLayout 				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		vkimgmembar_pres_und2prs[1].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_und2prs[1].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_und2prs[1].image 					= vkswap_img[1];
		vkimgmembar_pres_und2prs[1].subresourceRange 		= vkimgsubrange;

	for(int i = 0; i < 2; i++) {
		vr("vkBeginCommandBuffer", &vkres, 
			vkBeginCommandBuffer(vkcombuf_pres_init[i], &vkcombufbegin_info[i]) );

			rv("vkCmdPipelineBarrier");
				vkCmdPipelineBarrier (
					vkcombuf_pres_init[i],
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT,
					0, NULL, 0, NULL,
					1, &vkimgmembar_pres_und2prs[i] );

		vr("vkEndCommandBuffer", &vkres, 
			vkEndCommandBuffer(vkcombuf_pres_init[i]) ); }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "SUBMIT PRES_INIT");		/**/
	///////////////////////////////////////////////////

	for(int i = 0; i < 2; i++) {
		if(valid) {
			rv("vkcombuf_pres_init");
			VkSubmitInfo vksub_info[1];
				vksub_info[0].sType	= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				vksub_info[0].pNext					= NULL;
				vksub_info[0].waitSemaphoreCount	= 0;
				vksub_info[0].pWaitSemaphores		= NULL;
				vksub_info[0].pWaitDstStageMask		= NULL;
				vksub_info[0].commandBufferCount	= 1;
				vksub_info[0].pCommandBuffers		= &vkcombuf_pres_init[i];
				vksub_info[0].signalSemaphoreCount	= 0;
				vksub_info[0].pSignalSemaphores		= NULL;
			vr("vkQueueSubmit", &vkres, 
				vkQueueSubmit(vkq[0], 1, vksub_info, VK_NULL_HANDLE) ); } }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "RECORD PRES_LOOP");		/**/
	///////////////////////////////////////////////////

	VkAttachmentDescription vkattdesc;
		vkattdesc.flags 			= 0;
		vkattdesc.format 			= vksurf_fmt[SURF_FMT].format;
		vkattdesc.samples 			= VK_SAMPLE_COUNT_1_BIT;
		vkattdesc.loadOp 			= VK_ATTACHMENT_LOAD_OP_LOAD;
		vkattdesc.storeOp 			= VK_ATTACHMENT_STORE_OP_STORE;
		vkattdesc.stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkattdesc.stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkattdesc.initialLayout 	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkattdesc.finalLayout 		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference vkattref_pres;
		vkattref_pres.attachment 	= 0;
		vkattref_pres.layout 		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription vksubpassdesc_pres;
		vksubpassdesc_pres.flags 						= 0;
		vksubpassdesc_pres.pipelineBindPoint 			= VK_PIPELINE_BIND_POINT_GRAPHICS;
		vksubpassdesc_pres.inputAttachmentCount 		= 0;
		vksubpassdesc_pres.pInputAttachments 			= NULL;
		vksubpassdesc_pres.colorAttachmentCount 		= 1;
		vksubpassdesc_pres.pColorAttachments 			= &vkattref_pres;
		vksubpassdesc_pres.pResolveAttachments 			= NULL;
		vksubpassdesc_pres.pDepthStencilAttachment 		= NULL;
		vksubpassdesc_pres.preserveAttachmentCount 		= 0;
		vksubpassdesc_pres.pPreserveAttachments 		= NULL;

	VkRenderPassCreateInfo vkrendpass_info;
		vkrendpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	nf(&vkrendpass_info);
		vkrendpass_info.attachmentCount 	= 1;
		vkrendpass_info.pAttachments 		= &vkattdesc;
		vkrendpass_info.subpassCount 		= 1;
		vkrendpass_info.pSubpasses 			= &vksubpassdesc_pres;
		vkrendpass_info.dependencyCount 	= 0;
		vkrendpass_info.pDependencies 		= NULL;

	VkRenderPass vkrendpass;
	va("vkCreateRenderPass", &vkres, vkrendpass,
		vkCreateRenderPass(vkld[0], &vkrendpass_info, NULL, &vkrendpass) );

	VkFramebufferCreateInfo vkframebuff_info[2];
		vkframebuff_info[0].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	nf(&vkframebuff_info[0]);
		vkframebuff_info[0].renderPass 			= vkrendpass;
		vkframebuff_info[0].attachmentCount 	= 1;
		vkframebuff_info[0].pAttachments 		= &vkimgview[0];
		vkframebuff_info[0].width 				= vksurf_ables[0].currentExtent.width;
		vkframebuff_info[0].height 				= vksurf_ables[0].currentExtent.height;
		vkframebuff_info[0].layers 				= 1;
		vkframebuff_info[1].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	nf(&vkframebuff_info[1]);
		vkframebuff_info[1].renderPass 			= vkrendpass;
		vkframebuff_info[1].attachmentCount 	= 1;
		vkframebuff_info[1].pAttachments 		= &vkimgview[1];
		vkframebuff_info[1].width 				= vksurf_ables[0].currentExtent.width;
		vkframebuff_info[1].height 				= vksurf_ables[0].currentExtent.height;
		vkframebuff_info[1].layers 				= 1;

	VkFramebuffer vkframebuff[2];
	va("vkCreateFramebuffer", &vkres, vkframebuff[0],
		vkCreateFramebuffer(vkld[0], &vkframebuff_info[0], NULL, &vkframebuff[0]) );
	va("vkCreateFramebuffer", &vkres, vkframebuff[1],
		vkCreateFramebuffer(vkld[0], &vkframebuff_info[1], NULL, &vkframebuff[1]) );

	VkRenderPassBeginInfo vkrpbegininfo_pres[2];
		vkrpbegininfo_pres[0].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkrpbegininfo_pres[0].pNext 				= NULL;
		vkrpbegininfo_pres[0].renderPass 			= vkrendpass;
		vkrpbegininfo_pres[0].framebuffer 			= vkframebuff[0];
		vkrpbegininfo_pres[0].renderArea 			= vkrect2d;
		vkrpbegininfo_pres[0].clearValueCount 		= 1;
		vkrpbegininfo_pres[0].pClearValues 			= &vkclearval[0];
		vkrpbegininfo_pres[1].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkrpbegininfo_pres[1].pNext 				= NULL;
		vkrpbegininfo_pres[1].renderPass 			= vkrendpass;
		vkrpbegininfo_pres[1].framebuffer 			= vkframebuff[1];
		vkrpbegininfo_pres[1].renderArea 			= vkrect2d;
		vkrpbegininfo_pres[1].clearValueCount 		= 1;
		vkrpbegininfo_pres[1].pClearValues 			= &vkclearval[1];

	VkPipelineLayoutCreateInfo vkpipelay_info;
		vkpipelay_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	nf(&vkpipelay_info);
		vkpipelay_info.setLayoutCount 			= 0;
		vkpipelay_info.pSetLayouts 				= NULL;
		vkpipelay_info.pushConstantRangeCount 	= 0;
		vkpipelay_info.pPushConstantRanges 		= NULL;
	VkPipelineLayout vkpipelay;
	va("vkCreatePipelineLayout", &vkres, vkpipelay,
		vkCreatePipelineLayout(vkld[0], &vkpipelay_info, NULL, &vkpipelay) );

	VkGraphicsPipelineCreateInfo vkgfxpipe_info_pres;
		vkgfxpipe_info_pres.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	nf(&vkgfxpipe_info_pres);
		vkgfxpipe_info_pres.stageCount 				= 1;
		vkgfxpipe_info_pres.pStages 				= vkgfxpipe_ss_info;
		vkgfxpipe_info_pres.pVertexInputState 		= &vkpipevertinput_info;
		vkgfxpipe_info_pres.pInputAssemblyState 	= &vkpipeinputass_info;
		vkgfxpipe_info_pres.pTessellationState 		= NULL;
		vkgfxpipe_info_pres.pViewportState 			= &vkpipeviewport_info;
		vkgfxpipe_info_pres.pRasterizationState 	= &vkpiperastinfo;
		vkgfxpipe_info_pres.pMultisampleState 		= &vkpipems_info;
		vkgfxpipe_info_pres.pDepthStencilState 		= NULL;
		vkgfxpipe_info_pres.pColorBlendState 		= &vkpipecolblend;
		vkgfxpipe_info_pres.pDynamicState 			= NULL;
		vkgfxpipe_info_pres.layout 					= vkpipelay;
		vkgfxpipe_info_pres.renderPass 				= vkrendpass;
		vkgfxpipe_info_pres.subpass 				= 0;
		vkgfxpipe_info_pres.basePipelineHandle 		= VK_NULL_HANDLE;
		vkgfxpipe_info_pres.basePipelineIndex 		= -1;

	VkPipeline vkpipe_pres;
	va("vkCreateGraphicsPipelines", &vkres, vkpipe_pres,
		vkCreateGraphicsPipelines(vkld[0], VK_NULL_HANDLE, 1, &vkgfxpipe_info_pres, NULL, &vkpipe_pres) );

	VkMemoryRequirements vkmemreqs_swap;
	rv("vkGetImageMemoryRequirements");
		vkGetImageMemoryRequirements(vkld[0], vkswap_img[0], &vkmemreqs_swap);
		ov("memreq size", 			vkmemreqs_swap.size);
		ov("memreq alignment", 		vkmemreqs_swap.alignment);
		ov("memreq memoryTypeBits", vkmemreqs_swap.memoryTypeBits);

	VkBufferCreateInfo vkbuff_info_save;
		vkbuff_info_save.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	nf(&vkbuff_info_save);
		vkbuff_info_save.size 						= vkmemreqs_swap.size;
		vkbuff_info_save.usage 						= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vkbuff_info_save.sharingMode 				= VK_SHARING_MODE_EXCLUSIVE;
		vkbuff_info_save.queueFamilyIndexCount 		= 1;
		vkbuff_info_save.pQueueFamilyIndices 		= &GQF_IDX;

	VkBuffer vkbuff_save;
	va("vkCreateBuffer", &vkres, vkbuff_save,
		vkCreateBuffer(vkld[0], &vkbuff_info_save, NULL, &vkbuff_save) );

	int mem_index_save = UINT32_MAX;
	for(int i = 0; i < vkpd_memprops.memoryTypeCount; i++) {
		if( vkpd_memprops.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		&&	mem_index_save == UINT32_MAX ) {
			mem_index_save = i; } }
	ov("mem_index_save", mem_index_save);

	VkMemoryRequirements vkmemreqs_save;
	rv("vkGetBufferMemoryRequirements");
		vkGetBufferMemoryRequirements(vkld[0], vkbuff_save, &vkmemreqs_save);
		ov("memreq size", 			vkmemreqs_save.size);
		ov("memreq alignment", 		vkmemreqs_save.alignment);
		ov("memreq memoryTypeBits", vkmemreqs_save.memoryTypeBits);

	VkMemoryAllocateInfo vkmemallo_info_save;
		vkmemallo_info_save.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkmemallo_info_save.pNext			= NULL;
		vkmemallo_info_save.allocationSize	= vkmemreqs_save.size;
		vkmemallo_info_save.memoryTypeIndex	= mem_index_save;

	VkDeviceMemory vkdevmem_save;
	vr("vkAllocateMemory", &vkres, 
		vkAllocateMemory(vkld[0], &vkmemallo_info_save, NULL, &vkdevmem_save) );

	vr("vkBindBufferMemory", &vkres, 
		vkBindBufferMemory(vkld[0], vkbuff_save, vkdevmem_save, 0) );

	VkImageMemoryBarrier vkimgmembar_pres_prs2src[2];
		vkimgmembar_pres_prs2src[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_pres_prs2src[0].pNext 					= NULL;
		vkimgmembar_pres_prs2src[0].srcAccessMask 			= 0;
		vkimgmembar_pres_prs2src[0].dstAccessMask 			= 0;
		vkimgmembar_pres_prs2src[0].oldLayout 				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		vkimgmembar_pres_prs2src[0].newLayout 				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		vkimgmembar_pres_prs2src[0].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_prs2src[0].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_prs2src[0].image 					= vkswap_img[0];
		vkimgmembar_pres_prs2src[0].subresourceRange 		= vkimgsubrange;
		vkimgmembar_pres_prs2src[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_pres_prs2src[1].pNext 					= NULL;
		vkimgmembar_pres_prs2src[1].srcAccessMask 			= 0;
		vkimgmembar_pres_prs2src[1].dstAccessMask 			= 0;
		vkimgmembar_pres_prs2src[1].oldLayout 				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		vkimgmembar_pres_prs2src[1].newLayout 				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		vkimgmembar_pres_prs2src[1].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_prs2src[1].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_prs2src[1].image 					= vkswap_img[1];
		vkimgmembar_pres_prs2src[1].subresourceRange 		= vkimgsubrange;

	VkImageMemoryBarrier vkimgmembar_pres_src2dst[2];
		vkimgmembar_pres_src2dst[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_pres_src2dst[0].pNext 					= NULL;
		vkimgmembar_pres_src2dst[0].srcAccessMask 			= 0;
		vkimgmembar_pres_src2dst[0].dstAccessMask 			= 0;
		vkimgmembar_pres_src2dst[0].oldLayout 				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		vkimgmembar_pres_src2dst[0].newLayout 				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkimgmembar_pres_src2dst[0].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_src2dst[0].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_src2dst[0].image 					= vkswap_img[0];
		vkimgmembar_pres_src2dst[0].subresourceRange 		= vkimgsubrange;
		vkimgmembar_pres_src2dst[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_pres_src2dst[1].pNext 					= NULL;
		vkimgmembar_pres_src2dst[1].srcAccessMask 			= 0;
		vkimgmembar_pres_src2dst[1].dstAccessMask 			= 0;
		vkimgmembar_pres_src2dst[1].oldLayout 				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		vkimgmembar_pres_src2dst[1].newLayout 				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkimgmembar_pres_src2dst[1].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_src2dst[1].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_pres_src2dst[1].image 					= vkswap_img[1];
		vkimgmembar_pres_src2dst[1].subresourceRange 		= vkimgsubrange;

	VkImageMemoryBarrier vkimgmembar_work_src2sro[2];
		vkimgmembar_work_src2sro[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_work_src2sro[0].pNext 					= NULL;
		vkimgmembar_work_src2sro[0].srcAccessMask 			= 0;
		vkimgmembar_work_src2sro[0].dstAccessMask 			= 0;
		vkimgmembar_work_src2sro[0].oldLayout 				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		vkimgmembar_work_src2sro[0].newLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_work_src2sro[0].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_work_src2sro[0].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_work_src2sro[0].image 					= vkimg_work[0];
		vkimgmembar_work_src2sro[0].subresourceRange 		= vkimgsubrange;
		vkimgmembar_work_src2sro[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkimgmembar_work_src2sro[1].pNext 					= NULL;
		vkimgmembar_work_src2sro[1].srcAccessMask 			= 0;
		vkimgmembar_work_src2sro[1].dstAccessMask 			= 0;
		vkimgmembar_work_src2sro[1].oldLayout 				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		vkimgmembar_work_src2sro[1].newLayout 				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkimgmembar_work_src2sro[1].srcQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_work_src2sro[1].dstQueueFamilyIndex 	= GQF_IDX;
		vkimgmembar_work_src2sro[1].image 					= vkimg_work[1];
		vkimgmembar_work_src2sro[1].subresourceRange 		= vkimgsubrange;

	for(int i = 0; i < 2; i++) {
		vr("vkBeginCommandBuffer", &vkres, 
			vkBeginCommandBuffer(vkcombuf_pres[i], &vkcombufbegin_info[i]) );

			rv("vkCmdPipelineBarrier");
				vkCmdPipelineBarrier (
					vkcombuf_pres[i],
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT,
					0, NULL, 0, NULL,
					1, &vkimgmembar_pres_prs2src[i] );

			rv("vkCmdCopyImageToBuffer");
				vkCmdCopyImageToBuffer (
					vkcombuf_pres[i], vkswap_img[i], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					vkbuff_save, 1, &vkbuffimgcopy );

			rv("vkCmdPipelineBarrier");
				vkCmdPipelineBarrier (
					vkcombuf_pres[i],
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT,
					0, NULL, 0, NULL,
					1, &vkimgmembar_pres_src2dst[i] );

			rv("vkCmdBlitImage");
				vkCmdBlitImage (
					vkcombuf_pres[i], 
					vkimg_work[i], 	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					vkswap_img[i], 	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &vkimgblit, 	VK_FILTER_NEAREST );

			rv("vkCmdPipelineBarrier");
				vkCmdPipelineBarrier (
					vkcombuf_pres[i],
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT,
					0, NULL, 0, NULL,
					1, &vkimgmembar_work_src2sro[i] );

			rv("vkCmdBeginRenderPass");
				vkCmdBeginRenderPass (
					vkcombuf_pres[i], &vkrpbegininfo_pres[i], VK_SUBPASS_CONTENTS_INLINE );
				rv("vkCmdBindPipeline");
					vkCmdBindPipeline (
						vkcombuf_pres[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipe_pres );
				rv("vkCmdDraw");
					vkCmdDraw (
						vkcombuf_pres[i], 3, 1, 0, 0 );
			rv("vkCmdEndRenderPass");
				vkCmdEndRenderPass(vkcombuf_pres[i]);

		vr("vkEndCommandBuffer", &vkres, 
			vkEndCommandBuffer(vkcombuf_pres[i]) ); }

	  ///////////////////////////////////////////////////
	 /**/	hd("STAGE:", "MAIN LOOP");				/**/
	///////////////////////////////////////////////////

	int  idx 			= 0;
	int  pause 			= 0;

	int	 panning		= 0;
	int	 last_mx		= 0;
	int  smoothscale	= 0;
	int	 dbg_panel		= 0;
	int  mod_ctrl_l		= 1;
	int  last_press		= 0;
	int  vid_frm_idx	= 0;
	int  vid_frm_max	= 500;
	int	 send_img		= 0;

	int  SCR_count 		= 0;
	int  SCR_frameskip 	= 8;
	int  SCR_record 	= 0;
	int  SCR_make_vid	= 0;
	int  SCR_batchsize	= SCR_BATCH;
	int  SCR_save		= 0;
	int  SCR_save_count = 0;

	int  current_sec	= time(0);
	int  fps_freq 		= 1;
	int  fps_report 	= time(0) - fps_freq;
	int  fp_r_limit 	= 1;
    auto start_loop 	= std::chrono::high_resolution_clock::now();
    auto finish_loop 	= start_loop;
	auto start_frame 	= start_loop;
	auto finish_frame 	= start_loop;
	auto start_save 	= start_loop;
	auto finish_save 	= start_loop;

	uint32_t ubi[4];
	uint32_t ubv[48];
	uint32_t ev2[2];
	uint32_t ev4[4];
	uint32_t ev8[8];
	uint32_t sce[SPCONST_ENTRIES];

	int  update_ubiv 	= 0;
	int  update_sc 		= 0;
	int  set_scale		= 0;
	int  save_config	= 0;
	int  load_config	= 0;
	int  do_mutate		= 0;

	float mut = 0.2;

	pcd = new_pcd();
	PatternConfigData 		tgt = new_pcd();
	PatternConfigData_412 	pcdtmp;

	std::ifstream fload(loadfile.c_str(), std::ios::in | std::ios::binary);
    	fload.seekg (0, fload.end);
    	int f_len = fload.tellg();
		load_pcd_index = 0;
		pcd_load_size = 0;
		if( save_version <= 1616459498 ) 								{ pcd_load_size = sizeof(pcd340); }
		if( save_version >  1616459498 && save_version <= 1616831545 )	{ pcd_load_size = sizeof(pcd404); }
		if( save_version >  1616831545 ) 						 		{ pcd_load_size = sizeof(pcd408); }
		load_pcd_index = (f_len / pcd_load_size) - 1;

	fload.close();
	ov("Loaded Patterns", load_pcd_index + 1);

	int loaded_count = load_pcd_index;
	int flen_skip = 1;

	void* SCR_data;
	vr("vkMapMemory", &vkres, 
		vkMapMemory(vkld[0], vkdevmem_save, 0, VK_WHOLE_SIZE, 0, &SCR_data) );

	do {
    	start_loop 	= std::chrono::high_resolution_clock::now();
		current_sec = time(0);
		if( current_sec - fps_report >= fps_freq + 1) { fps_report = current_sec; }
		flen_skip = (((f_len / pcd_load_size)/100) > 10) ? ((f_len / pcd_load_size)/100) : 9;

		//	UI Controls
		if(valid) {
			send_img = 0;
/*
			send_img = 2;
			if(idx%32==0) {
				IMG_512_288 loadimg_data = load_video_frame(vid_frm_idx%vid_frm_max);
				rv("memcpy");
					memcpy(pvoid_memmap_para_loadimg, &loadimg_data, sizeof(loadimg_data));
				vid_frm_idx++; } /**/


			XQueryPointer(d, w, &wVoid, &wVoid, &siVoid, &siVoid, &siVoid, &siVoid, &uiVoid);
			mouse_info.run_cmd 	= 0;
			XSelectInput( d, w, xswa.event_mask );
			for(int pending_xe = 0; pending_xe < XPending(d); pending_xe++) {
				if(XCheckWindowEvent(d, w, xswa.event_mask, &xe)) {

					if( xe.type == 4 || xe.type == 5 || xe.type == 6 || xe.type == 2 ) {
						mouse_info.mouse_x 	= xe.xbutton.x;
						mouse_info.mouse_y 	= xe.xbutton.y; }

					if( xe.type == 3) { 
						//	L_CTRL Modifier: 			L_CTRL
						if( xe.xbutton.button == 50	 ) { 
							if( last_press == 50 ) { mouse_info.run_cmd = 1; }
							mod_ctrl_l = 1; }
						ov("Release Key", xe.xbutton.button); }

					if( xe.type == 4 || xe.type == 5) {
						if(xe.type == 4) {
							last_press = xe.xbutton.button;
							//	Set Config Savepoint		MMB
							if(xe.xbutton.button == 2) {
								panning = 0;
								SCR_save = (SCR_save > 150) ? 150 : SCR_save;
								mouse_info.run_cmd 	= 1;
								set_scale 			= 1;
								save_config 		= 1;
								f_len				= f_len + pcd_load_size;
								if( window_size.mode == 0 && window_size.divs == 2) {
									int		div_idx			= floor((mouse_info.mouse_x*window_size.divs)/(window_size.app_w))
															+ floor((mouse_info.mouse_y*window_size.divs)/(window_size.app_h))*window_size.divs;
									WSize 	ws_tmp;
											ws_tmp.app_w	= APP_W;
											ws_tmp.app_h	= APP_H;
											ws_tmp.divs		= 1;
											ws_tmp.mode 	= 3;
									if(div_idx == 0) { ws_tmp.mode = 1; }
									if(div_idx == 1) { ws_tmp.mode = 2; }
									if(div_idx == 2) { ws_tmp.mode = 0; }
									if(div_idx == 3) { ws_tmp.mode = 3; }
									pcdtmp.wsz_save = wsize_pack(ws_tmp);
								} else { pcdtmp.wsz_save = ub.wsize; }
								if(mod_ctrl_l == 0) { load_pcd_index = (f_len / pcd_load_size) - 1; } }

							if(xe.xbutton.button == 3) {
								if(!panning) {
									panning = 1;
									last_mx = mouse_info.mouse_x; } }
							//	Increase Mutation Strength	MWU
							if(xe.xbutton.button == 4) {
								if( mod_ctrl_l == 0 ) {
									mut = ((mut >= 0.002) ? ((mut >= 0.019) ? ((mut >= 1.0) ? mut + 0.5 : mut + 0.01 ) : mut + 0.001) : mut + 0.0001);
									mut = (mut >= 32.0) ? 32.0 : mut;
									mut = float(int(round(mut * 10000.0)) / 10000.0);
									ov("Mutation Rate", mut); }
								if( window_size.mode == 1 || mod_ctrl_l == 1) {
									mouse_info.run_cmd 	= 4;
									ub.zoom += float(0.05);
									ov("Zoom Scale", ub.zoom); }
								if(window_size.mode == 2 && mod_ctrl_l == 0) {
									ub.scale -= ub.scale * 0.05;
									ov("Scale", ub.scale); } }
							//	Decrease Mutation Strength	MWD
							if(xe.xbutton.button == 5) {
								if( mod_ctrl_l == 0 ) {
									mut = ((mut >= 0.002) ? ((mut >= 0.02) ? ((mut >= 1.5) ? mut - 0.5 : mut - 0.01 ) : mut - 0.001) : mut - 0.0001);
									mut = (mut <= 0.0001) ? 0.0001 : mut;
									mut = float(int(round(mut * 10000.0)) / 10000.0);
									ov("Mutation Rate", mut); }
								if( window_size.mode == 1 || mod_ctrl_l == 1 ) {
									mouse_info.run_cmd 	= 4;
									ub.zoom -= float(0.05);
									ub.zoom = (ub.zoom <= 0.0) ? 0.0 : ub.zoom;
									ov("Zoom Scale", ub.zoom); }
								if(window_size.mode == 2 && mod_ctrl_l == 0) {
									ub.scale += ub.scale * 0.05;
									ov("Scale", ub.scale); } }
							//	Load Prev Pattern			TBB
							if(xe.xbutton.button == 8) {
								int	shift_size 		= (mod_ctrl_l == 1) ? 1 : (flen_skip+1);
									load_config 	= 1;
									load_pcd_index 	= (load_pcd_index - shift_size + (f_len / pcd_load_size)) % (f_len / pcd_load_size);
									pcd 			= load_pcd(save_version, loadfile, load_pcd_index);
								ov("Loaded", load_pcd_index);
								ov("Loadfile", loaded_count); }
							//	Load Next Pattern			TBF
							if(xe.xbutton.button == 9) {
								int	shift_size 		= (mod_ctrl_l == 1) ? 1 : (flen_skip+1);
									load_config 	= 1;
									load_pcd_index 	= (load_pcd_index + shift_size) % (f_len / pcd_load_size);
									pcd 			= load_pcd(save_version, loadfile, load_pcd_index);
								ov("Loaded", load_pcd_index);
								ov("Loadfile", loaded_count); } }

						//	MouseButtons:	LMB,MMB,RMB,MWU,MWD,TBB,TBF
						if(	xe.xbutton.button == 1
						|| 	xe.xbutton.button == 2
						|| 	xe.xbutton.button == 3
						|| 	xe.xbutton.button == 4
						|| 	xe.xbutton.button == 5
						|| 	xe.xbutton.button == 8
						|| 	xe.xbutton.button == 9 	) {
							if(xe.type == 4 && (mod_ctrl_l == 0 || xe.xbutton.button == 1) ) { 
								mouse_info.mouse_c = xe.xbutton.button; }
							if(xe.type == 5) { 
								mouse_info.mouse_c = 0;
								if(xe.xbutton.button == 3) {
//									mouse_info.run_cmd 	= 1;
									panning = 0; } } } }

					if( xe.type == 2 ) {
						last_press = xe.xbutton.button;
						//	Pause Simulation: 			SPACEBAR
						if( xe.xbutton.button == 65	 ) {
							pause = (pause) ? 0 : 1;
							ov("Pause State", ((pause) ? "Paused" : "Running") ); }

						//	L_CTRL Modifier: 			L_CTRL
						if( xe.xbutton.button == 50	 ) { mod_ctrl_l = 0; }

						//	Toroidal Subdivisions:		1,2,3,4
						if( xe.xbutton.button == 10	 ) {
							if( window_size.divs == 1) {
								window_size.mode = (window_size.mode == 0) ? 1 : 0;	}
							window_size.divs 	= 1;
							mouse_info.run_cmd 	= 1;
							SCR_save 			= (SCR_save > 150) ? 150 : SCR_save; }
						if( xe.xbutton.button == 11	 ) { 
							window_size.divs 	= 2;
							window_size.mode 	= 0;
							mouse_info.run_cmd 	= 1; }
						if( xe.xbutton.button == 12	 ) { 
							window_size.divs 	= 4;
							window_size.mode 	= 0;
							mouse_info.run_cmd 	= 1; }
						if( xe.xbutton.button == 13	 ) { 
							window_size.divs 	= 8;
							window_size.mode 	= 0;
							mouse_info.run_cmd 	= 1; }
						if( xe.xbutton.button == 14	 ) { 
							window_size.divs 	= 1;
							window_size.mode 	= 2;
							mouse_info.run_cmd 	= 1; }
						if( xe.xbutton.button == 15	 ) { 
							window_size.divs 	= 1;
							window_size.mode 	= (window_size.mode == 3) ? 1 : 3;
							mouse_info.run_cmd 	= 1; }
						if( xe.xbutton.button == 49	 ) { 
							smoothscale 		= (smoothscale) ? 0 : 1; }
						if( xe.xbutton.button == 19	 ) { 
							dbg_panel 			= (dbg_panel) ? 0 : 1; }

					//	Load Prev Pattern			TBB
						if(xe.xbutton.button == 113) {
							int	shift_size 		= (mod_ctrl_l == 1) ? 1 : (flen_skip+1);
								load_config 	= 1;
								load_pcd_index 	= (load_pcd_index - shift_size + (f_len / pcd_load_size)) % (f_len / pcd_load_size);
								pcd 			= load_pcd(save_version, loadfile, load_pcd_index);
							ov("Loaded", load_pcd_index);
							ov("Loadfile", loaded_count); }
						//	Load Next Pattern			TBF
						if(xe.xbutton.button == 114) {
							int	shift_size 		= (mod_ctrl_l == 1) ? 1 : (flen_skip+1);
								load_config 	= 1;
								load_pcd_index 	= (load_pcd_index + shift_size) % (f_len / pcd_load_size);
								pcd 			= load_pcd(save_version, loadfile, load_pcd_index);
							ov("Loaded", load_pcd_index);
							ov("Loadfile", loaded_count); }

						if( xe.xbutton.button == 20 ) { 
							send_img = 1;
							IMG_512_288 loadimg_data = load_video_frame(vid_frm_idx%vid_frm_max);
							rv("memcpy");
								memcpy(pvoid_memmap_para_loadimg, &loadimg_data, sizeof(loadimg_data));
							vid_frm_idx++; }

						//	Recording Toggle:			Numpad ENTER
						if( xe.xbutton.button == 104 ) {
							SCR_record = (SCR_record) ? 0 : 1;
							ov("Record State", ((SCR_record) ? "Recording" : "Idle") ); }

						//	Make Video:					Numpad .
						if( xe.xbutton.button == 91  ) {
							SCR_record		= 0;
							pause			= 1;
							SCR_make_vid 	= 1;
							ov("Record State", ((SCR_record) ? "Recording" : "Idle") );
							ov("Output Video", SCR_count ); }

						//	Increase FrameSkip:			Numpad +
						if( xe.xbutton.button == 86  ) {
							SCR_frameskip = SCR_frameskip + 1;
							ov("FrameSkip", SCR_frameskip); }

						//	Decrease FrameSkip:			Numpad -
						if( xe.xbutton.button == 82  ) {
							SCR_frameskip = (SCR_frameskip > 1) ? SCR_frameskip - 1 : 1;
							ov("FrameSkip", SCR_frameskip); }

						//	Custom Injection			=
						if( xe.xbutton.button == 21 ) {
							update_ubiv 		= 1;
							update_sc 			= 1;
							mouse_info.run_cmd 	= 1;
							do_mutate 			= 5; }

						//	Mutate						E
						if( xe.xbutton.button == 26 ) {
							update_ubiv 		= 1;
							update_sc 			= 1;
							mouse_info.run_cmd 	= 1;
							do_mutate 			= 1; }


						//	Mutate NHs					U
						if( xe.xbutton.button == 30 ) {
							update_sc 			= 1;
							mouse_info.run_cmd 	= 1;
							do_mutate 			= 3; }

						//	Mutate Params				C
						if( xe.xbutton.button == 54 ) {
							update_ubiv 		= 1;
							mouse_info.run_cmd 	= 1;
							do_mutate 			= 2; }

						//	Load and Mutate	Params		V
						if( xe.xbutton.button == 55 ) {
							load_config 		= 1;
							update_ubiv 		= 1;
							mouse_info.run_cmd 	= 1;
							do_mutate 			= 2; }

						//	Set Target Pattern			T
						if( xe.xbutton.button == 28 ) { 
							tgt = pcd;
							std::cout << PCD_out(tgt); }

						//	Set Target Pattern			Y
						if( xe.xbutton.button == 29 ) {
							update_ubiv 		= 1;
							do_mutate 			= 4; }

						//	Mutate UBI					I
						if( xe.xbutton.button == 31 ) {
							update_ubiv 		= 1;
							mouse_info.run_cmd 	= 1;
							do_mutate 			= 6; }

						//	Increase Zoom				ARROW UP
						if( xe.xbutton.button == 111 ) {
							mut = ((mut >= 0.002) ? ((mut >= 0.019) ? ((mut >= 1.0) ? mut + 0.5 : mut + 0.01 ) : mut + 0.001) : mut + 0.0001);
							mut = (mut >= 32.0) ? 32.0 : mut;
							mut = float(int(round(mut * 10000.0)) / 10000.0);
							ov("Mutation Rate", mut); }

						//	Decrease Zoom				ARROW DOWN
						if( xe.xbutton.button == 116 ) {
							mut = ((mut >= 0.002) ? ((mut >= 0.02) ? ((mut >= 1.5) ? mut - 0.5 : mut - 0.01 ) : mut - 0.001) : mut - 0.0001);
							mut = (mut <= 0.0001) ? 0.0001 : mut;
							mut = float(int(round(mut * 10000.0)) / 10000.0);
							ov("Mutation Rate", mut); }

						//	Randomise all values		R
						if( xe.xbutton.button == 27 ) {
							ub.zoom				= 0.0;
							if(window_size.mode == 0) { ub.scale = (float(rand()%4096) / 32.0) + 16.0; }
							if(window_size.mode == 1) { ub.scale = 384.0; }
							if(window_size.mode == 2) { ub.scale = 384.0; }
							update_ubiv 		= 1;
							update_sc 			= 1;
							mouse_info.run_cmd 	= 1; }

						//	Clear						X
						if( xe.xbutton.button == 53 ) { mouse_info.run_cmd 	= 2; }

						//	Symmetrical Seed			Z
						if( xe.xbutton.button == 52 ) { mouse_info.run_cmd 	= 3; }

						//	Snap to div scale			F
						if( xe.xbutton.button == 41 ) { 
							SCR_save = (SCR_save > 150) ? 150 : SCR_save;
							mouse_info.run_cmd 	= 1;
							set_scale 			= 1; }

						if(update_ubiv) {
							ubi[0]  = ub.i0;  ubi[1]  = ub.i1;  ubi[2]  = ub.i2;  ubi[3]  = ub.i3;
							ubv[0]  = ub.v0;  ubv[1]  = ub.v1;  ubv[2]  = ub.v2;  ubv[3]  = ub.v3;
							ubv[4]  = ub.v4;  ubv[5]  = ub.v5;  ubv[6]  = ub.v6;  ubv[7]  = ub.v7;
							ubv[8]  = ub.v8;  ubv[9]  = ub.v9;  ubv[10] = ub.v10; ubv[11] = ub.v11;
							ubv[12] = ub.v12; ubv[13] = ub.v13; ubv[14] = ub.v14; ubv[15] = ub.v15;
							ubv[16] = ub.v16; ubv[17] = ub.v17; ubv[18] = ub.v18; ubv[19] = ub.v19;
							ubv[20] = ub.v20; ubv[21] = ub.v21; ubv[22] = ub.v22; ubv[23] = ub.v23;
							ubv[24] = ub.v24; ubv[25] = ub.v25; ubv[26] = ub.v26; ubv[27] = ub.v27;
							ubv[28] = ub.v28; ubv[29] = ub.v29; ubv[30] = ub.v30; ubv[31] = ub.v31;
							ubv[32] = ub.v32; ubv[33] = ub.v33; ubv[34] = ub.v34; ubv[35] = ub.v35;
							ubv[36] = ub.v36; ubv[37] = ub.v37; ubv[38] = ub.v38; ubv[39] = ub.v39;
							ubv[40] = ub.v40; ubv[41] = ub.v41; ubv[42] = ub.v42; ubv[43] = ub.v43;
							ubv[44] = ub.v44; ubv[45] = ub.v45; ubv[46] = ub.v46; ubv[47] = ub.v47; }

						//	Load Config Savepoint		Q
						if( xe.xbutton.button == 24 ) { load_config = 1; }

						//	Load Config & Randomise		TAB
						if( xe.xbutton.button == 23 ) {
							load_config = 1;
							do_mutate 	= 1; }

						//	Randomise all ub values		R
						if( xe.xbutton.button == 27 ) {
							for(int i = 0; i < SPCONST_ENTRIES; i++) 	{ sce[i] = rand()%UINT32_MAX; }
							for(int i = 0; i <  4; i++) 				{ ubi[i] = rand()%UINT32_MAX; }
							for(int i = 0; i < 48; i++) 				{ ubv[i] = rand()%UINT32_MAX; } }

						SCR_save = (SCR_save > 600) ? 600 : SCR_save;
						ov("Key", xe.xbutton.button); } } }

			if(dbg_panel) { 
				mouse_info.run_cmd 	= 14; }

			if(smoothscale) {
//				if(idx%SCR_frameskip == 1 && SCR_record && SCR_frameskip > 1) { mouse_info.run_cmd = 4; } 
//				else { if(idx%16 == 0) { mouse_info.run_cmd = 4; } }
				if(smoothscale == 1 && ub.scale > 0.5)	{ ub.scale -= ub.scale * 0.001; } else { smoothscale = 2; }
				if(smoothscale == 2) 					{ ub.scale += ub.scale * 0.005;
					if(ub.scale > 180.0) { smoothscale = 0; } } }

			if(load_config) {
				load_config 		= 0;
				mouse_info.run_cmd 	= 1;
				update_ubiv 		= 1;
				update_sc 			= 1;
				ub.scale 			= pcd.scl_save;
				ub.zoom 			= pcd.pzm_save;
				for(int i = 0; i < SPCONST_ENTRIES; i++) { sce[i] = pcd.scd_save[i]; }
				for(int i = 0; i < 4;  i++) { ubi[i] = pcd.ubi_save[i]; }
				for(int i = 0; i < 48; i++) { ubv[i] = pcd.ubv_save[i]; } }

			float mutscl = mut * 1.0;

			if(do_mutate) {
				if( do_mutate == 1
				||	do_mutate == 3 ) {
					for(int i = 0; i < SPCONST_ENTRIES; i++) { sce[i] = bit_flp(sce[i], 12); } }
				if( do_mutate == 1
				||	do_mutate == 2
				||	do_mutate == 6 ) {
					for(int i = 0; i <  4; i++) { ubi[i] = wrd_flp(ubi[i], 8, 4); }
					for(int i = 0; i <  4; i++) { ubi[i] = bit_flp(ubi[i], 64); } }
				if( do_mutate == 1
				||	do_mutate == 2 ) {
					//for(int i = 0; i < 48; i++) { ubv[i] = wrd_flp(ubv[i], 8, 8); }
					for(int i = 0; i < 48; i++) { ubv[i] = bit_flp(ubv[i], rand()%96+12); }
					int rnd_len = (rand()%32)/8;
					int rnd_off = rand()%(32-rnd_len);
					for(int i = 0; i < 48; i++) { if(rand()%(rand()%128+64) == 0) { ubv[i] = blk_set(ubv[i], rnd_len, rnd_off); } } }
				do_mutate = 0; }

			if(do_mutate == 4) {
				ov("Mutation Disabled", 4);
/*				do_mutate = 0;

				if(rand()%4 == 0) 	{ ub.scale = (ub.scale + tgt.scl_save * 0.05) / 1.05; }
				if(rand()%6 == 0) 	{ ub.scale = (ub.scale + pcd.scl_save * 0.05) / 1.05; }
				if(rand()%4 == 0) 	{ ub.zoom  = (ub.zoom  + tgt.pzm_save * 0.05) / 1.05; }
				if(rand()%6 == 0) 	{ ub.zoom  = (ub.zoom  + pcd.pzm_save * 0.05) / 1.05; }
				if(rand()%96 == 0) 	{ ub.scale = tgt.scl_save; }
				if(rand()%96 == 0) 	{ ub.scale = pcd.scl_save; }
				if(rand()%96 == 0) 	{ ub.zoom  = tgt.pzm_save; }
				if(rand()%96 == 0) 	{ ub.zoom  = pcd.pzm_save; }

				uint32_t ev4_tgt[48];
				uint32_t ev4_pcd[48];
				for(int i = 0; i < 48; i++) {
					eval4_unpack(ubv[i], ev4);
					eval4_unpack(tgt.ubv_save[i], ev4_tgt);
					eval4_unpack(pcd.ubv_save[i], ev4_pcd);
					for(int j = 0; j < 4; j++) {
						if(rand()%8 	== 0) 	{ ev4[j] = (ev4[j] * 15 + ev4_tgt[j]) / 16; }
						if(rand()%12 	== 0) 	{ ev4[j] = (ev4[j] * 15 + ev4_pcd[j]) / 16; }
						if(rand()%128 	== 0) 	{ ev4[j] = (ev4[j] + (rand()%(int(256.0*mutscl)+1) - rand()%(int(256.0*mutscl*0.5)+1)))%256; }
						if(rand()%256 	== 0) 	{ ev4[j] = ev4_pcd[j]; }
						if(rand()%256 	== 0) 	{ ev4[j] = ev4_tgt[j]; } }
					ubv[i] = eval4_pack(ev4); }

				if(rand()%12 == 0) 	{
					update_sc 			= 1;
					mouse_info.run_cmd 	= 1;
					for(int i = 0; i < SPCONST_ENTRIES; i++) {
						//if(rand()%4 	== 0) 	{ sce[i] = (sce[i] * 15 + tgt.scd_save[i]) / 16; }
						//if(rand()%6 	== 0) 	{ sce[i] = (sce[i] * 15 + pcd.scd_save[i]) / 16; }
						//if(rand()%12  	== 0) 	{ sce[i] = (sce[i] + (rand()%(int(16.0*mutscl)+1) - rand()%(int(16.0*mutscl*0.5)+1)))%16; }
						if(rand()%8 	== 0) 	{ sce[i] = tgt.scd_save[i]; }
						if(rand()%8 	== 0) 	{ sce[i] = pcd.scd_save[i]; } } }*/ }

			if(do_mutate == 5) {
				do_mutate = 0;
				ub.scale = 91.631157;
				ub.zoom = 0.0;
				uint32_t sce_inj[48] = {
					5, 1, 2, 7, 15, 7, 9, 15, 
					1, 10, 0, 6, 2, 10, 12, 7, 
					0, 4, 7, 3, 7, 8, 5, 15, 
					14, 6, 14, 7, 3, 9, 5, 0, 
					0, 4, 7, 3, 7, 8, 5, 15, 
					14, 6, 14, 7, 3, 9, 5, 0 };
				for(int i = 0; i < 48; i++) { sce[i] = sce_inj[i]; }
				uint32_t ubv_inj[48] = {
					3959981211, 2926676985, 2500758933, 3293554940, 
					2160965993, 3947745526, 1078807844, 4258892520, 
					74429364, 4018551356, 49760100, 1300800150, 
					2540414877, 4073103272, 3277090756, 1810631128, 
					1790538912, 273716168, 768864624, 1285389034, 
					1022171072, 746220962, 67464359, 3856300777, 
					4129972693, 3648990600, 1988687651, 817967097, 
					4247244940, 3828541660, 497473035, 4134368859, 
					1790538912, 273716168, 768864624, 1285389034, 
					1022171072, 746220962, 67464359, 3856300777, 
					4129972693, 3648990600, 1988687651, 817967097, 
					4247244940, 3828541660, 497473035, 4134368859 };
				for(int i = 0; i < 48; i++) { ubv[i] = ubv_inj[i]; } }

			if(set_scale) {
						set_scale 		= 0;
				float	xscale			= float(mouse_info.mouse_x) / float(window_size.app_w);
				int		div_idx			= floor((mouse_info.mouse_x*window_size.divs)/(window_size.app_w))
										+ floor((mouse_info.mouse_y*window_size.divs)/(window_size.app_h))*window_size.divs;
				float 	div_idx_scale 	= float(div_idx+1.0) / float(window_size.divs*window_size.divs);
				if( window_size.mode == 0 
				&& 	window_size.divs == 2) { ub.scale = ub.scale; }
				if( window_size.mode == 0
				&& 	window_size.divs != 2) { ub.scale = ub.scale * ((window_size.divs == 1) ? 1.0 : 2.0) * div_idx_scale; }
				if( window_size.mode == 1) { ub.scale = ub.scale * ((window_size.divs == 1) ? 1.0 : 2.0) 
													  + ub.scale * ((xscale - 0.5) * 2.0) * (1.0 / (1.0 + ub.zoom)); } }

			if(panning && mod_ctrl_l) {
				int 	mx_offset 	= mouse_info.mouse_x - last_mx;
				float	xscale		= float((window_size.app_w / 2.0) - float(mx_offset)) / float(window_size.app_w);
						ub.scale	= ub.scale + ub.scale * ((xscale - 0.5) * 2.0) * (1.0 / (1.0 + ub.zoom));
						last_mx		= mouse_info.mouse_x;
				if(mx_offset != 0) { 
					mouse_info.run_cmd 	= 4; } }
//					ov("MX Offset", mx_offset );
//					ov("MX Offset", float(mx_offset) / float(window_size.app_w) ); } }

			ub.minfo = minfo_pack(mouse_info);
			ub.wsize = wsize_pack(window_size);

			ub_wf.minfo = minfo_pack(mouse_info);
			ub_wf.wsize = wsize_pack(window_size);

			if(update_ubiv) {
				update_ubiv = 0;
				ub.i0  = ubi[0];  ub.i1  = ubi[1];  ub.i2  = ubi[2];  ub.i3  = ubi[3];
				ub.v0  = ubv[0];  ub.v1  = ubv[1];  ub.v2  = ubv[2];  ub.v3  = ubv[3];
				ub.v4  = ubv[4];  ub.v5  = ubv[5];  ub.v6  = ubv[6];  ub.v7  = ubv[7];
				ub.v8  = ubv[8];  ub.v9  = ubv[9];  ub.v10 = ubv[10]; ub.v11 = ubv[11];
				ub.v12 = ubv[12]; ub.v13 = ubv[13]; ub.v14 = ubv[14]; ub.v15 = ubv[15];
				ub.v16 = ubv[16]; ub.v17 = ubv[17]; ub.v18 = ubv[18]; ub.v19 = ubv[19];
				ub.v20 = ubv[20]; ub.v21 = ubv[21]; ub.v22 = ubv[22]; ub.v23 = ubv[23];
				ub.v24 = ubv[24]; ub.v25 = ubv[25]; ub.v26 = ubv[26]; ub.v27 = ubv[27];
				ub.v28 = ubv[28]; ub.v29 = ubv[29]; ub.v30 = ubv[30]; ub.v31 = ubv[31];
				ub.v32 = ubv[32]; ub.v33 = ubv[33]; ub.v34 = ubv[34]; ub.v35 = ubv[35];
				ub.v36 = ubv[36]; ub.v37 = ubv[37]; ub.v38 = ubv[38]; ub.v39 = ubv[39];
				ub.v40 = ubv[40]; ub.v41 = ubv[41]; ub.v42 = ubv[42]; ub.v43 = ubv[43];
				ub.v44 = ubv[44]; ub.v45 = ubv[45]; ub.v46 = ubv[46]; ub.v47 = ubv[47]; }

			if(update_sc) {
				update_sc = 0;
				scd.sc0  = sce[0];  scd.sc1  = sce[1];  scd.sc2  = sce[2];  scd.sc3  = sce[3];
				scd.sc4  = sce[4];  scd.sc5  = sce[5];  scd.sc6  = sce[6];  scd.sc7  = sce[7];
				scd.sc8  = sce[8];  scd.sc9  = sce[9];  scd.sc10 = sce[10]; scd.sc11 = sce[11];
				scd.sc12 = sce[12]; scd.sc13 = sce[13]; scd.sc14 = sce[14]; scd.sc15 = sce[15];
				scd.sc16 = sce[16]; scd.sc17 = sce[17]; scd.sc18 = sce[18]; scd.sc19 = sce[19];
				scd.sc20 = sce[20]; scd.sc21 = sce[21]; scd.sc22 = sce[22]; scd.sc23 = sce[23];
				scd.sc24 = sce[24]; scd.sc25 = sce[25]; scd.sc26 = sce[26]; scd.sc27 = sce[27];
				scd.sc28 = sce[28]; scd.sc29 = sce[29]; scd.sc30 = sce[30]; scd.sc31 = sce[31];
				scd.sc32 = sce[32]; scd.sc33 = sce[33]; scd.sc34 = sce[34]; scd.sc35 = sce[35];
				scd.sc36 = sce[36]; scd.sc37 = sce[37]; scd.sc38 = sce[38]; scd.sc39 = sce[39];
				scd.sc40 = sce[40]; scd.sc41 = sce[41]; scd.sc42 = sce[42]; scd.sc43 = sce[43];
				scd.sc44 = sce[44]; scd.sc45 = sce[45]; scd.sc46 = sce[46]; scd.sc47 = sce[47];

				if(loglevel != 0) { loglevel = loglevel * -1; }

				va("vkCreateGraphicsPipelines", &vkres, vkpipe_work[0],
					vkCreateGraphicsPipelines(vkld[0], VK_NULL_HANDLE, 1, &vkgfxpipe_info_work[0], NULL, &vkpipe_work[0]) );
				va("vkCreateGraphicsPipelines", &vkres, vkpipe_work[1],
					vkCreateGraphicsPipelines(vkld[0], VK_NULL_HANDLE, 1, &vkgfxpipe_info_work[1], NULL, &vkpipe_work[1]) );

				if(loglevel != 0) { loglevel = loglevel * -1; }

				for(int i = 0; i < 2; i++) {
					vr("vkBeginCommandBuffer", &vkres, 
						vkBeginCommandBuffer(vkcombuf_work[i], &vkcombufbegin_info[i]) );

						rv("vkCmdBeginRenderPass");
							vkCmdBeginRenderPass (
								vkcombuf_work[i], &vkrpbegininfo_work[i], VK_SUBPASS_CONTENTS_INLINE );

							rv("vkCmdBindPipeline");
								vkCmdBindPipeline (
									vkcombuf_work[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipe_work[i] );

							rv("vkCmdBindDescriptorSets");
								vkCmdBindDescriptorSets (
									vkcombuf_work[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipelay_work,
									0, 1, &vkdescset_work[i], 0, NULL );

							rv("vkCmdDraw");
								vkCmdDraw (
									vkcombuf_work[i], 3, 1, 0, 0 );

						rv("vkCmdEndRenderPass");
							vkCmdEndRenderPass(vkcombuf_work[i]);

					vr("vkEndCommandBuffer", &vkres, 
						vkEndCommandBuffer(vkcombuf_work[i]) ); }

				if(loglevel != 0) { loglevel = loglevel * -1; }
				if(loglevel != 0) { loglevel = loglevel * -1; }	}

			if(save_config) {
				ov("Save Config", "Yeah, do it!");
				save_config  = 0;
				pcd.scd_save[0]  = scd.sc0;  pcd.scd_save[1]  = scd.sc1;  pcd.scd_save[2]  = scd.sc2;  pcd.scd_save[3]  = scd.sc3;
				pcd.scd_save[4]  = scd.sc4;  pcd.scd_save[5]  = scd.sc5;  pcd.scd_save[6]  = scd.sc6;  pcd.scd_save[7]  = scd.sc7;
				pcd.scd_save[8]  = scd.sc8;  pcd.scd_save[9]  = scd.sc9;  pcd.scd_save[10] = scd.sc10; pcd.scd_save[11] = scd.sc11;
				pcd.scd_save[12] = scd.sc12; pcd.scd_save[13] = scd.sc13; pcd.scd_save[14] = scd.sc14; pcd.scd_save[15] = scd.sc15;
				pcd.scd_save[16] = scd.sc16; pcd.scd_save[17] = scd.sc17; pcd.scd_save[18] = scd.sc18; pcd.scd_save[19] = scd.sc19;
				pcd.scd_save[20] = scd.sc20; pcd.scd_save[21] = scd.sc21; pcd.scd_save[22] = scd.sc22; pcd.scd_save[23] = scd.sc23;
				pcd.scd_save[24] = scd.sc24; pcd.scd_save[25] = scd.sc25; pcd.scd_save[26] = scd.sc26; pcd.scd_save[27] = scd.sc27;
				pcd.scd_save[28] = scd.sc28; pcd.scd_save[29] = scd.sc29; pcd.scd_save[30] = scd.sc30; pcd.scd_save[31] = scd.sc31;
				pcd.scd_save[32] = scd.sc32; pcd.scd_save[33] = scd.sc33; pcd.scd_save[34] = scd.sc34; pcd.scd_save[35] = scd.sc35;
				pcd.scd_save[36] = scd.sc36; pcd.scd_save[37] = scd.sc37; pcd.scd_save[38] = scd.sc38; pcd.scd_save[39] = scd.sc39;
				pcd.scd_save[40] = scd.sc40; pcd.scd_save[41] = scd.sc41; pcd.scd_save[42] = scd.sc42; pcd.scd_save[43] = scd.sc43;
				pcd.scd_save[44] = scd.sc44; pcd.scd_save[45] = scd.sc45; pcd.scd_save[46] = scd.sc46; pcd.scd_save[47] = scd.sc47;
				pcd.ubi_save[0]  = ub.i0;  pcd.ubi_save[1]  = ub.i1;  pcd.ubi_save[2]  = ub.i2;  pcd.ubi_save[3]  = ub.i3;
				pcd.ubv_save[0]  = ub.v0;  pcd.ubv_save[1]  = ub.v1;  pcd.ubv_save[2]  = ub.v2;  pcd.ubv_save[3]  = ub.v3;
				pcd.ubv_save[4]  = ub.v4;  pcd.ubv_save[5]  = ub.v5;  pcd.ubv_save[6]  = ub.v6;  pcd.ubv_save[7]  = ub.v7;
				pcd.ubv_save[8]  = ub.v8;  pcd.ubv_save[9]  = ub.v9;  pcd.ubv_save[10] = ub.v10; pcd.ubv_save[11] = ub.v11;
				pcd.ubv_save[12] = ub.v12; pcd.ubv_save[13] = ub.v13; pcd.ubv_save[14] = ub.v14; pcd.ubv_save[15] = ub.v15;
				pcd.ubv_save[16] = ub.v16; pcd.ubv_save[17] = ub.v17; pcd.ubv_save[18] = ub.v18; pcd.ubv_save[19] = ub.v19;
				pcd.ubv_save[20] = ub.v20; pcd.ubv_save[21] = ub.v21; pcd.ubv_save[22] = ub.v22; pcd.ubv_save[23] = ub.v23;
				pcd.ubv_save[24] = ub.v24; pcd.ubv_save[25] = ub.v25; pcd.ubv_save[26] = ub.v26; pcd.ubv_save[27] = ub.v27;
				pcd.ubv_save[28] = ub.v28; pcd.ubv_save[29] = ub.v29; pcd.ubv_save[30] = ub.v30; pcd.ubv_save[31] = ub.v31;
				pcd.ubv_save[32] = ub.v32; pcd.ubv_save[33] = ub.v33; pcd.ubv_save[34] = ub.v34; pcd.ubv_save[35] = ub.v35;
				pcd.ubv_save[36] = ub.v36; pcd.ubv_save[37] = ub.v37; pcd.ubv_save[38] = ub.v38; pcd.ubv_save[39] = ub.v39;
				pcd.ubv_save[40] = ub.v40; pcd.ubv_save[41] = ub.v41; pcd.ubv_save[42] = ub.v42; pcd.ubv_save[43] = ub.v43;
				pcd.ubv_save[44] = ub.v44; pcd.ubv_save[45] = ub.v45; pcd.ubv_save[46] = ub.v46; pcd.ubv_save[47] = ub.v47;
				pcd.scl_save	 = ub.scale;
				pcd.pzm_save	 = ub.zoom;

				std::string 	savefile = "fbk/save_" + timestamp + ".vkpat";
				std::ofstream 	fout_local(savefile.c_str(), std::ios::out | std::ios::binary | std::ios::app);
					fout_local.write( (const char*)&pcd, sizeof(pcd) );
					fout_local.close();

								savefile = "fbk/save_global.vkpat";
				std::ofstream 	fout_global(savefile.c_str(), std::ios::out | std::ios::binary | std::ios::app);
					fout_global.write( (const char*)&pcd, sizeof(pcd) );
					fout_global.close();

				std::cout << PCD_out(pcd); } }

		//	Video Creation
		if(valid && SCR_make_vid && SCR_count > 0) {
			rv("SystemCLI: Make Thumbnail Image");
				int thumbnail_index = (SCR_count / 20) + 150;
					thumbnail_index = (thumbnail_index < SCR_count) ? thumbnail_index : SCR_count / 2;
				std::string thumbnail_image 	= "out/IMG" + std::to_string(thumbnail_index) + ".PAM";
				std::string first_image 		= "out/IMG" + std::to_string(0) + ".PAM";
				std::string cli_cmd_thumb 		= "cp '" + thumbnail_image + "' '" + first_image + "'";
				system(cli_cmd_thumb.c_str());

			rv("SystemCLI: Run 'make_video.sh'");
				system("./make_video.sh");

			rv("SystemCLI: Remove Output Images");
				for(int i = 0; i < SCR_count; i++) {
					std::string imgfile = "out/IMG" + std::to_string(i) + ".PAM";
					std::string cli_cmd_remove 	= "rm '" + imgfile + "'";
					system(cli_cmd_remove.c_str()); }

			ov("Video Creation", "Complete!" );

			SCR_count 		= 0;
			SCR_make_vid 	= 0; }

		//	Image Generation
		if (!pause) {
			if(valid) {
				rv("nanosleep(NS_DELAY)");
					nanosleep((const struct timespec[]){{0, NS_DELAY}}, NULL);

				iv("i", idx, idx);

				ub.frame 	= uint32_t(idx);
				ub_wf.frame = uint32_t(idx);
    			start_frame = std::chrono::high_resolution_clock::now();

				rv("memcpy");
					memcpy(pvoid_memmap_work, &ub, sizeof(ub));

				rv("memcpy");
					memcpy(pvoid_memmap_para, &ub_wf, sizeof(ub_wf));

				vr("vkResetFences", &vkres, 
					vkResetFences(vkld[0], 1, vkfence_aqimg) );

				if(valid) {
					vr("vkAcquireNextImageKHR", &vkres, 
						vkAcquireNextImageKHR(vkld[0], vkswap[0], UINT64_MAX, vksemaph_image[0], 
							VK_NULL_HANDLE, &aqimg_idx[0]) );
						iv("aqimg_idx", aqimg_idx[0], idx);
						iv("vkswap_img", vkswap_img[aqimg_idx[0]], idx);

					if(valid && send_img <= 1) {
						rv("vkcombuf_para");
						VkSubmitInfo vksub_info[1];
							vksub_info[0].sType	= VK_STRUCTURE_TYPE_SUBMIT_INFO;
							vksub_info[0].pNext					= NULL;
							vksub_info[0].waitSemaphoreCount	= 0;
							vksub_info[0].pWaitSemaphores		= NULL;
							vksub_info[0].pWaitDstStageMask		= NULL;
							vksub_info[0].commandBufferCount	= 1;
							vksub_info[0].pCommandBuffers		= &vkcombuf_para[aqimg_idx[0]];
							vksub_info[0].signalSemaphoreCount	= 0;
							vksub_info[0].pSignalSemaphores		= NULL;
						vr("vkQueueSubmit", &vkres, 
							vkQueueSubmit(vkq[0], 1, vksub_info, VK_NULL_HANDLE) ); }

					if(valid && send_img) {
						rv("vkcombuf_para_loadimg");
						VkSubmitInfo vksub_info[1];
							vksub_info[0].sType	= VK_STRUCTURE_TYPE_SUBMIT_INFO;
							vksub_info[0].pNext					= NULL;
							vksub_info[0].waitSemaphoreCount	= 0;
							vksub_info[0].pWaitSemaphores		= NULL;
							vksub_info[0].pWaitDstStageMask		= NULL;
							vksub_info[0].commandBufferCount	= 1;
							vksub_info[0].pCommandBuffers		= &vkcombuf_para_loadimg[aqimg_idx[0]];
							vksub_info[0].signalSemaphoreCount	= 0;
							vksub_info[0].pSignalSemaphores		= NULL;
						vr("vkQueueSubmit", &vkres, 
							vkQueueSubmit(vkq[0], 1, vksub_info, VK_NULL_HANDLE) ); }

					if(valid) {
						rv("vkcombuf_work");
						VkSubmitInfo vksub_info[1];
							vksub_info[0].sType	= VK_STRUCTURE_TYPE_SUBMIT_INFO;
							vksub_info[0].pNext					= NULL;
							vksub_info[0].waitSemaphoreCount	= 0;
							vksub_info[0].pWaitSemaphores		= NULL;
							vksub_info[0].pWaitDstStageMask		= NULL;
							vksub_info[0].commandBufferCount	= 1;
							vksub_info[0].pCommandBuffers		= &vkcombuf_work[aqimg_idx[0]];
							vksub_info[0].signalSemaphoreCount	= 0;
							vksub_info[0].pSignalSemaphores		= NULL;
						vr("vkQueueSubmit", &vkres, 
							vkQueueSubmit(vkq[0], 1, vksub_info, VK_NULL_HANDLE) ); }

					if(valid) {
						rv("vkcombuf_pres");
						VkSubmitInfo vksub_info[1];
							vksub_info[0].sType	= VK_STRUCTURE_TYPE_SUBMIT_INFO;
							vksub_info[0].pNext	= NULL;
							vksub_info[0].waitSemaphoreCount	= 1;
							vksub_info[0].pWaitSemaphores		= vksemaph_image;
							vksub_info[0].pWaitDstStageMask		= &qsubwait;
							vksub_info[0].commandBufferCount	= 1;
							vksub_info[0].pCommandBuffers		= &vkcombuf_pres[aqimg_idx[0]];
							vksub_info[0].signalSemaphoreCount	= 1;
							vksub_info[0].pSignalSemaphores		= vksemaph_rendr;
						vr("vkQueueSubmit", &vkres, 
							vkQueueSubmit(vkq[0], 1, vksub_info, vkfence_aqimg[0]) ); }
						
					if(valid) {
						do {
							vr("vkWaitForFences <100ms>", &vkres, 
								vkWaitForFences(vkld[0], 1, vkfence_aqimg, VK_TRUE, 100000000) );
						} while (vkres[vkres.size()-1] == VK_TIMEOUT);

						VkPresentInfoKHR vkpresent_info[1];
							vkpresent_info[0].sType 	= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
							vkpresent_info[0].pNext 	= NULL;
							vkpresent_info[0].waitSemaphoreCount	= 1;
							vkpresent_info[0].pWaitSemaphores 		= vksemaph_rendr;
							vkpresent_info[0].swapchainCount 		= 1;
							vkpresent_info[0].pSwapchains 			= vkswap;
							vkpresent_info[0].pImageIndices 		= aqimg_idx;
							vkpresent_info[0].pResults 				= NULL;

						if(valid) {
							vr("vkQueuePresentKHR", &vkres, 
								vkQueuePresentKHR(vkq[0], &vkpresent_info[0]) ); } } }

				if(fps_report == current_sec) {
					auto finish_frame	= std::chrono::high_resolution_clock::now();
					std::string ftime 	= std::to_string(
						std::chrono::duration_cast<std::chrono::nanoseconds>(finish_frame-start_frame).count()) + " ns, "
										+ std::to_string(
						int(1000000000.0 / std::chrono::duration_cast<std::chrono::nanoseconds>(finish_frame-start_frame).count())) + " FPS, i"
						+  std::to_string(idx);
					ov("Frame", ftime); } }

			if( valid && idx%SCR_frameskip == 0 && SCR_record ) {
				if( SCR_record ) { SCR_save = 0; }
				if( SCR_count%SCR_batchsize == 0 && SCR_count > 0 && SCR_batchsize > 0 ) { pause = 1; }

				std::string imgfile;
				if(SCR_save == 1 && 0) {
					imgfile = "scr/SCR_"	+ std::to_string(idx)
							+ "_" 			+ timestamp
							+ "_" 			+ std::to_string(SCR_save_count)
							+ ".PAM"; SCR_save_count++; } else {
					imgfile = "out/IMG" + std::to_string(SCR_count)
							+ ".PAM"; SCR_count++; }
				ov("Save Image", imgfile);

				std::ofstream file(imgfile.c_str(), std::ios::out | std::ios::binary);
					file 	<<	"P7" 												<< "\n"
						 	<< 	"WIDTH "	<< vksurf_ables[0].currentExtent.width 	<< "\n"
						 	<< 	"HEIGHT "	<< vksurf_ables[0].currentExtent.height	<< "\n"
						 	<< 	"DEPTH "	<< "4"									<< "\n"
						 	<< 	"MAXVAL "	<< "255"								<< "\n"
						 	<< 	"TUPLTYPE "	<< "RGB_ALPHA"							<< "\n"
						 	<< 	"ENDHDR"	<< "\n";
					file.write(
						(const char*)SCR_data,
						vksurf_ables[0].currentExtent.width * vksurf_ables[0].currentExtent.height * 4 );
				file.close();

				if(SCR_save == 1 && 0) {
					std::string scr_convert = "convert '" + imgfile + "' '" + imgfile + ".png'";
					system(scr_convert.c_str());
					if(window_size.divs == 1) {
						std::string scr_cp = "cp '" + imgfile + ".png' 'prs/" + imgfile + ".png'";
						system(scr_cp.c_str()); }
					std::string scr_remove 	= "rm '" + imgfile + "'";
					system(scr_remove.c_str()); } }

			SCR_save = (SCR_save > 0) ? SCR_save - 1 : 2500;
			idx++;

		} else { nanosleep((const struct timespec[]){{0, 100000000}}, NULL); }

		if(loglevel != 0 && idx == 2) { loglevel = loglevel * -1; }

		if(fps_report == current_sec) { fps_report--; }
	} while (valid && (idx < TEST_CYCLES || TEST_CYCLES < 1));

	  ///////////////////////////////////////////////
	 /**/	hd("STAGE:", "EXIT APPLICATION");	/**/
	///////////////////////////////////////////////

	if(!valid) { hd("STAGE:", "ABORTED"); }

	rv("XCloseDisplay");
		XCloseDisplay(d);

	rv("return");
		return 0;
}



