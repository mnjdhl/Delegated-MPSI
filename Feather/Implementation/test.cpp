
/*

- a test that runs both update and PSI computation in Feather protocol.

*/
//**********************************************************************

#include "Client.h"
#include "common.hpp"

const size_t ONE_MB = 1000000;

std::ofstream open_stats_file(std::string filename) {
	std::ofstream lfile(filename, std::ios::app);
	if (!lfile.is_open()) {
		std::cerr << "Error: Unable to open results file " << filename << "\n";
	}
	return lfile;
}

size_t calculateStringSize(const std::string& str) {
    return sizeof(str) + (str.capacity() >= sizeof(std::string) ? str.capacity() + 1 : 0);
}

size_t calculateSizeofMap(const std::unordered_map<std::string, int>& myMap) {
    size_t size = sizeof(myMap); // Base unordered_map structure

    for (const auto& [key, value] : myMap) {
        size += sizeof(std::pair<const std::string, int>); // Size of each node
        size += key.capacity(); // Account for dynamically allocated string memory
    }

    return size;
}

size_t sizeOfBigIntList(bigint *lst, int n) {
	size_t size = 0;

	// r: Array of mpz_t* pointers
	size += sizeof(bigint) * n;

	// Size of each mpz_t instance (metadata + actual number storage)
	for (size_t i = 0; i < n; i++) {
		size += (mpz_sizeinbase(lst[i], 2) + 7) / 8; // Byte size of actual number
	}

	return size;
}

size_t sizeOfBigIntTable(bigint **tab, int r_rows, int r_cols) {
	size_t size = 0;

	// r: Array of mpz_t* pointers
	size += sizeof(bigint*) * r_rows;

	// Size of each mpz_t instance (metadata + actual number storage)
	for (size_t i = 0; i < r_rows; i++) {
		if (tab[i] != nullptr) {
			size += sizeof(bigint) * r_cols; // Metadata for each mpz_t
			for (size_t j = 0; j < r_cols; j++) {
				size += (mpz_sizeinbase(tab[i][j], 2) + 7) / 8; // Byte size of actual number
			}
		}
	}

	return size;
}

size_t calSizeofOutsourcingData(Client_Dataset *db, int tabsz) {

	/*
	struct Client_Dataset{
		Polynomial* poly;// an array of polynomials
		bigint* labels;// an array of labels
		bigint* BF;//array of blinded bloom filters
		unordered_map <string, int> label_Index_map;
		string  client_ID;
	};
	*/
	size_t size = 0;

	size += sizeof(db->poly->val_size) + sizeOfBigIntList(db->poly->values, db->poly->val_size);

	
	size += sizeOfBigIntList(db->labels, tabsz);
	if (db->client_ID=="B_ID")
		size += sizeOfBigIntList(db->BF, tabsz);
	else
		size += sizeOfBigIntList(db->BF, 2);

	size += calculateSizeofMap(db->label_Index_map);
	size += calculateStringSize(db->client_ID);
	return size;
}

size_t calSizeofServerResultData(Server_Result *res, int r_rows, int r_cols) {
	/*
	struct Server_Result{

		bigint** result;// an array of permuted bins of a hash table.
		bigint* BF;// an array of blinded Bloom filters.
	};
	*/

	size_t size = 0;

	size += sizeOfBigIntTable(res->result, r_rows, r_cols);
	size += sizeOfBigIntList(res->BF, r_rows);

	return size;
}

size_t calSizeofCompPermReqData(CompPerm_Request *cpr, int r_rows, int r_cols) {
	/*
	struct CompPerm_Request{
		string id;
		bigint**r;//blinded blinding factors
		uint8_t label_key_[AES::DEFAULT_KEYLENGTH];
		uint8_t label_iv[AES::BLOCKSIZE];
		bigint shuffle_key_;
	};
	*/

	size_t size = 0;

    // Fixed-size members
    size += sizeof(cpr->label_key_);
    size += sizeof(cpr->label_iv);
    size += sizeof(bigint); // Metadata size of shuffle_key_

    // String size
    size += calculateStringSize(cpr->id);

	size += sizeOfBigIntTable(cpr->r, r_rows, r_cols);

    // Size of shuffle_key_ number storage
    size += (mpz_sizeinbase(cpr->shuffle_key_, 2) + 7) / 8;

    return size;
}

size_t calSizeofGrantCompInfoData(GrantComp_Info **gci, int n, int r_rows, int r_cols) {
	/*
	struct GrantComp_Info{
		string* id;// the result reciepent id is in id[0].
		uint8_t seed[AES::DEFAULT_KEYLENGTH];
		uint8_t iv[AES::BLOCKSIZE];
		bigint** pm; // permutation map
	};
	*/

	size_t size = 0;

	for (int i=0; i<n;i++) {
		// Fixed-size members
		size += sizeof(gci[i]->seed);
		size += sizeof(gci[i]->iv);


		// String size
		for (int j=0;j<2;j++) {
			size += calculateStringSize(gci[i]->id[j]);
		}

		size += sizeOfBigIntTable(gci[i]->pm, r_rows, r_cols);
		/*if (gci[i]->id[0]=="B_ID")
			size += sizeOfBigIntTable(gci[i]->pm, r_rows, r_cols);
		else
			size += sizeOfBigIntTable(gci[i]->pm, r_rows, 2);
		*/
		//size += sizeOfBigIntTable(gci[i]->pm, r_rows, 2);
	}

    return size;
}


//**********************************************************************
// - Function description: generates a set of random bigintegers,
// and ensures that the values are smaller than the public moduli and unequal to x-coordinates.
bigint* gen_randSet (int size, int max_bitsize, bigint* pubModuli, bigint* x_points, int xpoint_size){

	int counter = 0;
	Random rd;
	mpz_t *pr_val;
	pr_val=(mpz_t*)malloc(size * sizeof(mpz_t));
	unordered_map <string, int> map;
	string s_val;
	int max_bytesize = max_bitsize;
	gmp_randstate_t rand;
	bigint ran;
	rd.init_rand3(rand, ran, max_bytesize);
	bigint temp;
	mpz_init(temp);
	bool duplicated = false;
	for(int i = 0; i < size; i++){
		mpz_urandomb(temp, rand, max_bitsize);
		mpz_mod(temp, temp, pubModuli[0]);
		/*
		for(int k=0;k<counter; k++){
			if(mpz_cmp(pr_val[k],temp)==0)
			duplicated=true;
			}
			*/
		while (mpz_cmp(temp, pubModuli[0]) > 0 || duplicated == true){ // ensures the elements are smaller than the public moduli.
			mpz_init(temp);
			mpz_urandomb(temp, rand, max_bitsize);
			//extra checks-- ensures they are distinc.
			/*
			for(int k=0;k<counter; k++){
			if(mpz_cmp(pr_val[k],temp)==0){
			duplicated=true;break;
			}
			else{duplicated=false;
			}
			}
			*/
			for(int j = 0; j < xpoint_size; j++){ //checks the random element is not equal to any x_points.
				if(mpz_cmp(temp, x_points[j]) == 0){
					mpz_init(temp);
					mpz_urandomb(temp, rand, max_bitsize);
					for(int k=0;k<counter; k++){
						if(mpz_cmp(pr_val[k], temp) == 0){
							duplicated = true; break;
						}
						else{
							duplicated = false;
						}
					}
				}
			}
		}
		mpz_init_set(pr_val[i], temp);
		counter++;
		//s_val.clear();
		//s_val = mpz_get_str(NULL, 10, temp);
		//map.insert(make_pair(s_val, 1));
	}
	return pr_val;
}

Options &g_options = *(new Options());

int main(int argc, char* argv[]) {

	auto stfile = open_stats_file("stats1.out");
    // Parse options
    auto opt = parse_options(argc, argv);
    if (!opt) return 1;

    g_options = *opt;
	display_options(g_options);
	if (!validate_options(g_options)) {
		return 1;
	}
	int number_of_experiments = g_options.number_of_experiments;
	int number_of_clients = g_options.number_of_clients;
	int pub_mod_bitsize = g_options.pub_mod_bitsize;
	int max_setsize = g_options.max_setsize;
	int table_length = g_options.table_length;
	int bucket_max_load = g_options.bucket_max_load;
	int interSec_size = g_options.interSec_size;
	int xsize = g_options.xsize;


	/*
	int pub_mod_bitsize = 40;
	int max_setsize = 1024;//524288;//131072; //65536; // 1024; // 1048576;  // 4096;
	int table_length = 30;//5242 // 2621; // 30; // 41943; //122 ;
	int bucket_max_load = 100;
	int interSec_size = 1;//131072;
	if(interSec_size > max_setsize){cout<<"interSec_size > max_setsize"<<endl; return 0;}
	int xsize = 201;// Note that the number of x is determined by bucket_max_load
	if(xsize < (2 * bucket_max_load) + 1) {
		cout<<"\nxsize must be greater than 2*bucket_max_load)+1, reset it\n";
		return 0;
	}
	int number_of_experiments = 1;
	int number_of_clients = 2;
	*/
	double temp_req = 0;
	double temp_grant = 0;
	double temp_res = 0;
	double temp_intersect = 0;
	double temp_out = 0;
	double sum = 0;
	double start_out_1 = 0;
	double end_out_1 = 0;
	double sum_1 = 0;
	double diff_b = 0;
	size_t outsourcing_comm_size=0, comp_req_comm_size=0, server_result_size=0, grant_comp_info_size=0;
	for(int l = 0;l < number_of_experiments; l++){
		Server serv(xsize, number_of_clients, pub_mod_bitsize, max_setsize , bucket_max_load, table_length);
		Server * serv_ptr (& serv);
		int elem_bit_size = 100;
		bigint* pub_mod = serv.get_pubModuli();
		// Assigning random values to two sets a and b.
		cout<<"\n----------------------------------------------------------------\n";
		cout<<"\t Set_size:       "<<max_setsize<<endl;
		cout<<"\t Pub_mod_bitsize:  "<<pub_mod_bitsize<<endl;
		cout<<"\t Bucket_max_load:  "<<bucket_max_load<<endl;
		cout<<"\t Table_length:     "<<table_length<<endl;
		cout<<"\t Public modulous:   "<<pub_mod[0]<<endl;
		cout<<"\t Number of Clients :   "<<number_of_clients <<endl;
		cout<<"\n----------------------------------------------------------------\n";
		int t1 , t2;
		mpz_t *aa, *bb, **a;
		a = (mpz_t**)malloc((number_of_clients - 1) * sizeof(mpz_t));
		bb = (mpz_t*)malloc(max_setsize * sizeof(mpz_t));
		cout<<"\n------ Generating two random sets with distinct values and unequal to x_points"<<endl;
		bb = gen_randSet (max_setsize, elem_bit_size, serv.get_pubModuli(), serv.get_xpoints(t2), xsize);
		for(int i = 0;i < number_of_clients - 1; i++){
			a[i] = (mpz_t*)malloc(max_setsize * sizeof(mpz_t));
			a[i] = gen_randSet (max_setsize, elem_bit_size, serv.get_pubModuli(), serv.get_xpoints(t1), xsize);
		}
		bigint *x_p = serv.get_xpoints(t1);
		for(int j = 0; j < number_of_clients -1; j++){
			for(int i = 0; i < interSec_size; i++){
				mpz_set(a[j][i], bb[i]);
			}
		}
		//define the authorizers: A_i
		Client **A_;
		string A_IDs[number_of_clients - 1];
		A_ = new Client* [number_of_clients - 1];
		for(int j = 0; j < number_of_clients - 1; j++){
			A_[j] = new Client(serv_ptr, a[j], max_setsize);
			A_IDs[j]="A_"+to_string(j)+"ID";
		}
		Client B(serv_ptr, bb, max_setsize);
		string b_id = "B_ID";
		bigint label;
		cout<<"\n----------------- Client B outsourcing -----------------"<<endl;
		double start_out_b = clock();
		//B.outsource_db(b_id);
		outsourcing_comm_size +=B.outsource_db(b_id); // via calSizeofOutsourcingData()
		double end_out_b = clock();
		diff_b = end_out_b - start_out_b;
		cout<<"\n----------------- Clients are outsourcing -----------------"<<endl;
		free(bb);
		for(int j = 0; j < number_of_clients - 1; j++){
			cout<<"\n client "<<j<<" is outsourcing"<<endl;
			A_[j]->outsource_db(A_IDs[j]);
		}
		for(int j = 0; j < number_of_clients -1; j++){
			for(int i = 0; i < max_setsize; i++){
				mpz_clear(a[j][i]);
			}
			free(a[j]);
		}
		free (a);
		Random rd_;
		bigint* labels;
		int num_of_exper = 1;
		Random rd_1;
		bigint *temp_9;
		int size_;
		
		//-------Update----------
		bigint *temp;
		temp = (mpz_t*)malloc(1 * sizeof(mpz_t));
		temp= gen_randSet (1, elem_bit_size,serv.get_pubModuli(), serv.get_xpoints(t1), xsize);
		cout<<"\n inserting:"<<temp[0]<<endl;
		cout<<"\n++++++++++++++++++++++"<<endl;
		string ss = B.update(temp[0], "insertion", label, "B_ID");
 		string sss = B.update(temp[0], "deletion", label, "B_ID");
 		cout<<ss<<endl;
 		cout<<sss<<endl;
		//-----------Set Intersection------------
		bigint **q;
		int* sz;
		cout<<"\n---- Gennerate the Computation Request"<<endl;
		uint8_t B_tk[AES::DEFAULT_KEYLENGTH];
		uint8_t  B_tIV [AES::BLOCKSIZE];
		memset(B_tk, 0x00, (AES::DEFAULT_KEYLENGTH) + 1);
		memset(B_tIV, 0x00, (AES::BLOCKSIZE) + 1);
		double start_req = clock();
		CompPerm_Request* req = B.gen_compPerm_req(B_tk, B_tIV);
		double end_req = clock();
		temp_req += end_req - start_req;
		
		/********************/
		comp_req_comm_size += calSizeofCompPermReqData(req, serv.get_table_size(), xsize);
		/********************/

		cout<<"\n---- Grant the Computation Done"<<endl;
		GrantComp_Info** ptr;
		ptr = new GrantComp_Info*[number_of_clients - 1];
		bigint ***Q;
		Q = (mpz_t***)malloc((number_of_clients - 1) * sizeof(mpz_t));
		double start_grant;
		double end_grant;
		for(int j = 0; j < number_of_clients - 1; j++){
			if(j == 0){start_grant = clock();
			}
			ptr[j] = A_[j]->grant_comp(req, Q[j], true);
			if(j == 0){end_grant = clock();
			}
		}
		for(int j = 0; j < number_of_clients - 1; j++){
			A_[j]->free_client();
			delete A_[j];
		}
		temp_grant += end_grant - start_grant;
		cout<<"\n***---- Server-side Result Computation."<<endl;
		double start_res=clock();
		Server_Result * res = serv.compute_result(ptr, B_tk, B_tIV);
		double end_res = clock();
		temp_res += end_res - start_res;

		/********************/
		server_result_size += calSizeofServerResultData(res, serv.get_table_size(), xsize);
		grant_comp_info_size += calSizeofGrantCompInfoData(ptr, number_of_clients-1, serv.get_table_size(), 2); //xsize);
		/********************/
		
		//-----Just to free some memory----
		cout<<"\n cleanging the server"<<endl;
		serv.free_server();
		cout<<"\n---- Client-side Result Retirieval"<<endl;

		double start_intersect = clock();
		vector<string>  final_res = B.find_intersection(res, sz , Q, number_of_clients);
		double end_intersect = clock();
		temp_intersect += end_intersect - start_intersect;
		cout<<"\n\n\t======= Result ======="<<endl;
		for(int i = 0; i < final_res.size(); i++){
			cout<<"\n\nFinal_res "<<i + 1<<": "<<final_res[i]<<endl;
		}
	}
	cout<<"\n===================="<<endl;
	cout<<"\n\n\t============= Run time ==================="<<endl;
	double out = diff_b  /number_of_experiments;
	float out_time = out / (double) CLOCKS_PER_SEC;
	cout<<"Outsourcing comm cost = "<<(outsourcing_comm_size/ONE_MB)/out_time<<"\n";
	cout<<"\n\n Outsourcing-- time:"<<out_time<<endl;
	double com_req = temp_req / number_of_experiments;
	float req_time = com_req / (double) CLOCKS_PER_SEC;
	cout<<"Computation Request comm cost = "<<(comp_req_comm_size/ONE_MB)/req_time<<"\n";
	cout<<"\n\n Computation Request-- time:"<<req_time<<endl;
	double grant = temp_grant / number_of_experiments;
	float grant_time = grant / (double) CLOCKS_PER_SEC;
	cout<<"Computation Grant comm cost = "<<(grant_comp_info_size/ONE_MB)/grant_time<<"\n";
	cout<<"\n\n Computation Grant-- time:"<<grant_time<<endl;
	double res_= temp_res / number_of_experiments;
	float res_time = res_ / (double) CLOCKS_PER_SEC;
	cout<<"\n\n Server Computation-- time:"<<res_time<<endl;
	double inter = temp_intersect / number_of_experiments;
	float inter_time = inter / (double) CLOCKS_PER_SEC;
	cout<<"\n\n Find intersection-- time:"<<inter_time<<endl;
	cout<<"\n\n\t============================================"<<endl;
	size_t tot_data_sent = (outsourcing_comm_size + comp_req_comm_size + server_result_size + grant_comp_info_size)/ONE_MB;
	double avg_tot_data_sent = double(tot_data_sent)/double(number_of_experiments);
	cout<<"Average total data sent = "<<avg_tot_data_sent<<" MB"<<"\n";
	stfile<<max_setsize<<", "<<number_of_clients<<", "<<avg_tot_data_sent<<"\n";
	stfile.close();
//-----------End of Set intersection------------
	// Free options
	delete &g_options;
return 0;

}
//**********************************************************************
