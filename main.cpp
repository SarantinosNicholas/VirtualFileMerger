#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
using namespace std;

class virtual_data_file
{//this class allows functions to access the multiple files as if they were one
	vector<unsigned long long> 
		file_sizes,
		start_pos,
		seek_positions;
	
	vector<unique_ptr<ifstream>> file_streams;
	
	unsigned long long 
		total_file_size,
		external_seek_pos;
	
	unsigned int total_files_number;
	
	size_t get_file_size(string name)
	{
		ifstream file(name.c_str(), ios::binary | ios::in);//sets the init pos at the end

		if(!file.is_open()) {/*error here*/}

		file.seekg (0, file.end);
		size_t length = file.tellg();
		file.seekg (0, file.beg);

		file.close();

		return length;
	}

	int find_stream_id_based_on_start_pos(unsigned long long now_start_pos)
	{
		for(int i = 0; i < total_files_number; i++)
			if(start_pos[i] <= now_start_pos && start_pos[i]+file_sizes[i] > now_start_pos)
				return i;
		
		/*error here*/
		
		return 0;
	}
	
	void do_read(char* output, unsigned long long length, unsigned long long now_start_pos)
	{
		bool first_round = true;

		unsigned long long 
			bytes_buff = 0,
			data_pos = 0,
			bytes_remaining = length,
			read_until = now_start_pos+length;

		int file_id = find_stream_id_based_on_start_pos(now_start_pos);

		while(bytes_remaining > 0)
		{
			unsigned long long file_reaches_till = file_sizes[file_id]+start_pos[file_id];

			bytes_remaining = (read_until < file_reaches_till ? 0 : read_until-file_reaches_till);//bytes we will need to read from all other files excluding this one

			unsigned long long 
				length_to_read_now = (first_round ? (length-bytes_remaining) : (bytes_buff > file_sizes[file_id] ? file_sizes[file_id] : bytes_buff)),
				file_now_start_from = (first_round ? now_start_pos-start_pos[file_id] : 0);
			
			if(seek_positions[file_id] != file_now_start_from)//no need to seek, we are already there
				file_streams[file_id]->seekg(file_now_start_from, ios::beg);//seek to the start of the file
			
			file_streams[file_id]->read((char*)(&output[data_pos]), length_to_read_now);
			seek_positions[file_id] = file_now_start_from+length_to_read_now;
			
			if(seek_positions[file_id] == file_sizes[file_id])
			{
				file_streams[file_id]->seekg(0, ios::beg);
				seek_positions[file_id] = 0;
			}
			
			if(seek_positions[file_id] > file_sizes[file_id]) {/*error here*/}
			if(file_streams[file_id]->eof()) {/*error here*/}
			if(length_to_read_now != file_streams[file_id]->gcount()) {/*error here*/}
			
			data_pos += length_to_read_now;
			bytes_buff = bytes_remaining;
			file_id++;
			
			first_round = false;
		}
		
		external_seek_pos = now_start_pos+length;
		if(external_seek_pos == total_file_size) external_seek_pos = 0;
	}
	
	void do_init(string *files_array, unsigned int total_files_number_in)
	{
		total_files_number = total_files_number_in;
		unsigned long long size_ = 0;
		
		for(int i = 0; i < total_files_number; i++)
		{
			string now = files_array[i];
			
			unsigned long long file_size_now = get_file_size(now);
			
			file_sizes.push_back(file_size_now);
			start_pos.push_back(size_);
			seek_positions.push_back(0);
			
			size_ += file_size_now;
			unique_ptr<ifstream> fs_now(new ifstream(now, ios::in | ios::binary));
			
			if(!fs_now->is_open()) {/*error here*/}
			
			file_streams.push_back(move(fs_now));
		}
		
		total_file_size = size_;
		external_seek_pos = 0;
	}
	
public:
	virtual_data_file(string *files_array, unsigned int total_files_number_in)
	{
		do_init(files_array, total_files_number_in);
	}
	
	~virtual_data_file()
	{
		for(int i = 0; i < total_files_number; i++)
			file_streams[i]->close();
	}
	
	void read(void* output, unsigned long long length, unsigned long long now_start_pos = ULLONG_MAX)
	{
		if(length == 0) {/*error here*/}
		else if((now_start_pos == ULLONG_MAX ? external_seek_pos : now_start_pos)+length > total_file_size) {/*error here*/}
		else do_read((char*)output, length, (now_start_pos == ULLONG_MAX ? external_seek_pos : now_start_pos));
	}
	
	unsigned long long get_size()
	{
		return total_file_size;
	}
};

int main()
{
	//small demostartion

	string files[] = {
		"C:\\some_files\\a.txt",
		"C:\\some_files\\b.txt",
		"C:\\some_files\\c.txt",
		"C:\\some_files\\d.txt"
	};

	virtual_data_file data_files(files, sizeof(files)/sizeof(string));

	size_t max_size = data_files.get_size();//do not read more than that!
	char* str = new char[max_size];
	data_files.read(str, max_size);//read

	cout << str << "\n";//show what you got

	return 0;
}
