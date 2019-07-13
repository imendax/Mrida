// SWAMI KARUPPASWAMI THUNNAI

#pragma warning(disable: 4996)

#include "trendcpp.h"
#include <experimental/filesystem>
#include <sqlite_modern_cpp.h>



trendcpp::trendcpp()
{
	
}


trendcpp::~trendcpp()
{
}

std::string trendcpp::hash_file_to_string(std::string file_location)
{
	if (!std::experimental::filesystem::exists(file_location)) return "";
	Tlsh th;
	///////////////////////////////////////
	// 1. How big is the file?
	///////////////////////////////////////
	FILE *fd = fopen(file_location.c_str(), "r");
	if (fd == NULL)
		return "";
	int ret = 1;
	int sizefile = 0;

	fseek(fd, 0L, SEEK_END);
	sizefile = ftell(fd);

	fclose(fd);

	if (sizefile < MIN_DATA_LENGTH)
		return "";

	///////////////////////////////////////
	// 2. allocate the memory
	///////////////////////////////////////
	unsigned char* data = (unsigned char*)malloc(sizefile);
	if (data == NULL) {
		fprintf(stderr, "out of memory...\n");
		exit(0);
	}

	///////////////////////////////////////
	// 3. read the file
	///////////////////////////////////////
#ifdef WINDOWS
	// Handle differently for Windows because the fread function in msvcr80.dll has a bug
	// and it does not always read the entire file.
	if (!read_file_win(file_location.c_str(), sizefile, data)) {
		free(data);
		return "";
	}
#else
	fd = fopen(fname, "r");
	if (fd == NULL) {
		free(data);
		return(ERROR_READING_FILE);
	}

	ret = fread(data, sizeof(unsigned char), sizefile, fd);
	fclose(fd);

	if (ret != sizefile) {
		fprintf(stderr, "fread %d bytes from %s failed: only %d bytes read\n", sizefile, fname, ret);
		return(ERROR_READING_FILE);
	}
#endif

	///////////////////////////////////////
	// 4. calculate the digest
	///////////////////////////////////////
	th.final(data, sizefile, 0);

	///////////////////////////////////////
	// 5. clean up and return
	///////////////////////////////////////
	free(data);
	if (th.getHash() == NULL || th.getHash()[0] == '\0') {
		return "";
	}
	return th.getHash();
}

const Tlsh * trendcpp::hash_file(std::string file_location)
{
	if (!std::experimental::filesystem::exists(file_location)) return nullptr;
	Tlsh th;
	///////////////////////////////////////
	// 1. How big is the file?
	///////////////////////////////////////
	FILE *fd = fopen(file_location.c_str(), "r");
	if (fd == NULL)
		return nullptr;
	int ret = 1;
	int sizefile = 0;

	fseek(fd, 0L, SEEK_END);
	sizefile = ftell(fd);

	fclose(fd);

	if (sizefile < MIN_DATA_LENGTH)
		return nullptr;

	///////////////////////////////////////
	// 2. allocate the memory
	///////////////////////////////////////
	unsigned char* data = (unsigned char*)malloc(sizefile);
	if (data == NULL) {
		fprintf(stderr, "out of memory...\n");
		exit(0);
	}

	///////////////////////////////////////
	// 3. read the file
	///////////////////////////////////////
#ifdef WINDOWS
	// Handle differently for Windows because the fread function in msvcr80.dll has a bug
	// and it does not always read the entire file.
	if (!read_file_win(file_location.c_str(), sizefile, data)) {
		free(data);
		return nullptr;
	}
#else
	fd = fopen(fname, "r");
	if (fd == NULL) {
		free(data);
		return(ERROR_READING_FILE);
	}

	ret = fread(data, sizeof(unsigned char), sizefile, fd);
	fclose(fd);

	if (ret != sizefile) {
		fprintf(stderr, "fread %d bytes from %s failed: only %d bytes read\n", sizefile, fname, ret);
		return(ERROR_READING_FILE);
	}
#endif

	///////////////////////////////////////
	// 4. calculate the digest
	///////////////////////////////////////
	th.final(data, sizefile, 0);

	///////////////////////////////////////
	// 5. clean up and return
	///////////////////////////////////////
	free(data);
	if (th.getHash() == NULL || th.getHash()[0] == '\0') {
		return nullptr;
	}
	return &th;
}

void trendcpp::add_threat_to_database(unsigned long int id, std::string tlsh_hash, std::string threat_name, unsigned long file_size, unsigned int file_type)
{
	try {
		sqlite::database threat_database("threat_db.db");
		threat_database << "create table if not exists threat(id unsigned bigint primary key, threat_hash text, threat_name text, threat_size unsigned int, threat_type unsigned int);";
		threat_database << "insert into threat(id, threat_hash, threat_name, threat_size, threat_type) values(?, ?, ?, ?, ?)" << id << tlsh_hash << threat_name << file_size << file_type;
	}
	catch (std::exception &e)
	{
		std::cout << e.what();
	}
}

int trendcpp::similarity_distance(std::string hash_one, std::string hash_two)
{
	Tlsh t1;
	Tlsh t2;
	int err1;
	int err2;
	err1 = t1.fromTlshStr(hash_one.c_str());
	err2 = t2.fromTlshStr(hash_two.c_str());
	if (err1 || err2) return -1;
	return t1.totalDiff(&t2);
}

unsigned int trendcpp::mime_to_id(std::string mime_type)
{
	
	sqlite::database db("threat_db.db");
	db << "create table if not exists mime_table(mime text, id int)";
	int count = 0;
	db << "select count(id) from mime_table where mime=?" << mime_type >> count;
	int max = 0;
	db << "select max(id) from mime_table limit 1" >> max;
	if (count == 0)
	{
		max++;
		db << "insert into mime_table(mime, id) values(?, ?)" << mime_type << max;
		return max;
	}
	else
	{
		unsigned int id;
		db << "select id from mime_table where mime=? limit 1" << mime_type >> id;
		return id;
	}
	return 0;
}

long trendcpp::matching_hash_from_threat_db(std::string tlsh_hash, std::string file_type, long file_size_minimum, unsigned long file_size_maximum)
{
	long matched_id = -1;
	sqlite::database threat_table("threat_db.db");
	threat_table << "create table if not exists threat(id unsigned bigint primary key, threat_hash text, threat_name text, threat_size unsigned int, threat_type unsigned int);";
	unsigned int file_id = mime_to_id(file_type);
	threat_table << "select id, threat_hash from threat where threat_size>=? and threat_size<=? and threat_type=?"
		<< file_size_minimum << file_size_maximum << file_id >> [&] (unsigned long id, std::string threat_hash)
		{
			if (similarity_distance(tlsh_hash, threat_hash) < 20)
			{
				matched_id = id;
			}
		};
	return matched_id;
}
