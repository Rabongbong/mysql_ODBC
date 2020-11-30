#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;
MYSQL_FIELD *filed;
int u_id;  // u_id를 담아놓고 있다.


void bid_ending(){

	char * query = (char *)malloc(sizeof(char) * 220);
	char * query_trans = (char *)malloc(sizeof(char) * 100);

	sprintf(query, " select i_id, item.u_id, bid_history.u_id, most_recent_bid_price from item join bid_history using(i_id) where i_id not in(select i_id from Transaction) and now() > bid_ending_date and most_recent_bid_price=bid_price;");
	if (mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	res = mysql_store_result(conn);
	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
	sprintf(query_trans, "insert into Transaction(i_id, seller, buyer, sold_price) values(%s, %s, %s, %s)", row[0], row[1], row[2], row[3]);
	if (mysql_query(conn, query_trans)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}
	}
	free(query);
	free(query_trans);
}

void bid_history(char * price, int i_id){

	char * query = (char *)malloc(sizeof(char) *150);
	char * tran_query = (char *)malloc(sizeof(char) *100);
	char * query_bid = (char *)malloc(sizeof(char) *150);

	if(!strcmp(price, "buy")){
		// update item
		sprintf(query, "update item set most_recent_bid_price = (select * from (select buy_it_now_price from item where i_id = %d) as T) where i_id= %d;", i_id, i_id);
		if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}
		// update Transaction
		sprintf(tran_query, "update Transaction set buyer = %d where i_id= %d;", u_id, i_id);
		if (mysql_query(conn, tran_query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}
		// update bid_history
		sprintf(query_bid, "insert into bid_history(u_id, i_id, bid_price) values(%d, %d, (select buy_it_now_price from item where i_id = %d));", u_id, i_id, i_id);
		if (mysql_query(conn, query_bid)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}
	}
	else{
		sprintf(query, "update item set most_recent_bid_price = %s where i_id =%d;", price, i_id);
		if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}

		sprintf(query_bid, "insert into bid_history(u_id, i_id, bid_price) values(%d, %d, %s);", u_id, i_id, price);
		if (mysql_query(conn, query_bid)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}
	}

	free(query);
	free(tran_query);
	free(query_bid);
}


void Watched(int * item, int num_row){  //insert into Watch_list

	char * query = (char *)malloc(sizeof(char) *100);
	printf("%d", num_row);
	for(int i=0; i<num_row; i++){
		sprintf(query, "insert ignore into Watch_list(u_id, i_id) values(%d, %d);", u_id, item[i]);
		if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}
	}
	free(query);
}



void Search_category(){

	int cate;
	int num_row;
	int i;
	char * bid = (char *)malloc(sizeof(char) * 8);
	char * category = (char *)malloc(sizeof(char) * 15);
	char * query = (char *)malloc(sizeof(char) * 610);
	char * bid_price = (char *)malloc(sizeof(char)*8);
	int * item;

	printf("----< Search items by category >\n");
	printf("     (1) Electronics\n     (2) Books\n     (3) Home\n     (4) Clothing\n     (5) Sporting Goods\n     Your choice: ");
	scanf("%d", &cate);
	switch(cate){
		case 1:
			sprintf(category, "Electronics");
			break;
		case 2:
			sprintf(category, "Books");
			break;
		case 3:
			sprintf(category, "Home");
			break;
		case 4:
			sprintf(category, "Clothing");
			break;
		case 5:
			sprintf(category, "Sporting Goods");
	}

	sprintf(query, "with count_bid(i_id, value) as (select i_id, count(i_id) from bid_history group by i_id) select i_id, if(count_bid.value is NULL, 0, count_bid.value), description, most_recent_bid_price, date_posted, bid_ending_date, buy_it_now_price, if(name is NULL, 'Nobody', name) from count_bid natural right outer join item natural left join (select i_id, bid_price, name from bid_history natural join (select i_id, max(bid_price) as bid_price from User natural join bid_history group by i_id) as temp natural join User) as W where category= \'%s\' and i_id not in (select i_id from Transaction);", category);
		 
	if (mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	res = mysql_store_result(conn);
	num_row = mysql_num_rows(res);
	printf("%d", num_row);


	item = (int *)malloc(sizeof(int) * num_row);
	printf("----< Search results: Category >\n");
	i=0;
	while(1){
		
		row = mysql_fetch_row(res);
	
		if(!row)
			break;
		item[i] = atoi(row[0]);     //i_id
		printf("[%d]\n", i+1);
		printf("description: %s\n", row[2]);  // description
		printf("status: %s bids\n", row[1]);  //bid_count
		printf("current bidding price: %s\n", row[3]);  // most_recent_bid_price
		printf("current highest bidder: %s\n", row[7]);
		printf("buy-it-now-price: %s\n", row[6]); // buy_it_now_price
		printf("date posted: %s\n", row[4]);  // date_posted
		printf("bid ending date: %s\n", row[5]);  // bid_ending_date
		i++;
	}

	Watched(item, num_row);
	printf("--- Which item do you want to bid? (Enter the number or 'B' to go back to the previous menu) : ");
	scanf("%s", bid);
	if(bid[0] == 'B')
		return;
	printf("--- Bidding price? (Enter the price or 'buy' to pay for the buy-it-now price) : ");
	scanf("%s", bid_price);
	bid_history(bid_price, item[atoi(bid)-1]);
	
	free(item);
	free(category);
	free(query);
	free(bid_price);
}



void Search_keyword(){
	int i;
	int num_row;
	char * bid = (char *)malloc(sizeof(char) * 8);
	char * keyword = (char *)malloc(sizeof(char) * 30);
	char * query = (char *)malloc(sizeof(char) * 610);
	char * bid_price = (char *)malloc(sizeof(char)*8);
	int * item;

	printf("----< Search items by description keyword >\n---- Search keyword : ");
	scanf("%s", keyword);
	sprintf(query, "with count_bid(i_id, value) as (select i_id, count(i_id) from bid_history group by i_id) select i_id, if(count_bid.value is NULL, 0, count_bid.value), description, most_recent_bid_price, date_posted, bid_ending_date, buy_it_now_price, if(name is NULL, 'Nobody', name) from count_bid natural right outer join item natural left join (select i_id, bid_price, name from bid_history natural join (select i_id, max(bid_price) as bid_price from User natural join bid_history group by i_id) as temp natural join User) as W where description like \'%%%s%%\' and i_id not in (select i_id from Transaction);", keyword);
		 
	if (mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}


	res = mysql_store_result(conn);
	num_row = mysql_num_rows(res);
	printf("%d", num_row);

	item = (int *)malloc(sizeof(int) * num_row);
	printf("----< Search results: keyword search >\n");
	i=0;
	while(1){
		row = mysql_fetch_row(res);
	
		if(!row)
			break;

		item[i] = atoi(row[0]);
		//num_bids = num_of_bids(item[i]);
		printf("[%d]\n", i+1);
		printf("description: %s\n", row[2]);  // description
		printf("status: %s bids\n", row[1]);
		printf("current bidding price: %s\n", row[3]);  // most_recent_bid_price
		printf("current highest bidder: %s\n", row[7]);
		printf("buy-it-now-price: %s\n", row[6]);
		printf("date posted: %s\n", row[4]);  // date_posted
		printf("bid ending date: %s\n", row[5]);  // bid_ending_date
		i++;
	}

	Watched(item, num_row);
	printf("--- Which item do you want to bid? (Enter the number or 'B' to go back to the previous menu) : ");
scanf("%s", bid);
	if(bid[0] == 'B')
		return;
	printf("--- Bidding price? (Enter the price or 'buy' to pay for the buy-it-now price) : ");
	scanf("%s", bid_price);
	bid_history(bid_price, item[atoi(bid)-1]);
	
	free(item);
	free(keyword);
	free(query);
	free(bid_price);
}

void Search_seller(){

	int i;
	int num_row;
	char * bid = (char *)malloc(sizeof(char) * 8);
	char * seller = (char *)malloc(sizeof(char) * 30);
	char * query = (char *)malloc(sizeof(char) * 600);
	char * bid_price = (char *)malloc(sizeof(char)*10);
	int * item;

	printf("----< Search items by Seller >\n---- Search Seller : ");
	scanf("%s", seller);
	sprintf(query, "with count_bid(i_id, value) as (select i_id, count(i_id) from bid_history group by i_id) select i_id, if(count_bid.value is NULL, 0, count_bid.value), description, most_recent_bid_price, date_posted, bid_ending_date, buy_it_now_price, if(W.name is NULL, 'Nobody', W.name) from count_bid natural right outer join item natural join User natural right outer join (select i_id, bid_price, name from bid_history natural join (select i_id, max(bid_price) as bid_price from User natural join bid_history group by i_id) as temp natural join User) as W where User.name like \'%%%s%%\';", seller);
		 
	if (mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	res = mysql_store_result(conn);
	num_row = mysql_num_rows(res);
	printf("%d", num_row);

	item = (int *)malloc(sizeof(int) * num_row);
	printf("----< Search results: keyword search >\n");
	i=0;
	while(1){
		row = mysql_fetch_row(res);
	
		if(!row)
			break;

		item[i] = atoi(row[0]);
		printf("[%d]\n", i+1);
		printf("description: %s\n", row[2]);  // description
		printf("status: %s bids\n", row[1]);
		printf("current bidding price: %s\n", row[3]);  // most_recent_bid_price
		printf("current highest bidder: %s\n", row[7]);
		printf("buy-it-now-price: %s\n", row[6]);  
		printf("date posted: %s\n", row[4]);  // date_posted
		printf("bid ending date: %s\n", row[5]);  // bid_ending_date
		i++;
	}

	Watched(item, num_row);
	printf("--- Which item do you want to bid? (Enter the number or 'B' to go back to the previous menu) : ");
	scanf("%s", bid);
	if(bid[0] == 'B')
		return;
	printf("--- Bidding price? (Enter the price or 'buy' to pay for the buy-it-now price) : ");
	scanf("%s", bid_price);
	bid_history(bid_price, item[atoi(bid)-1]);

	free(item);
	free(seller);
	free(query);
	free(bid_price);
	
}


void Search_date(){

	int i;
	int num_row;
	char * bid = (char *)malloc(sizeof(char) * 8);
	char * date = (char *)malloc(sizeof(char) * 30);
	char * query = (char *)malloc(sizeof(char) * 610);
	char * bid_price = (char *)malloc(sizeof(char)*10);
	int * item;

	printf("----< Search items by Date >\n---- Search Date_posted(Please enter in this format yyyy-mm-dd): ");
	scanf("%s", date);
	sprintf(query, "with count_bid(i_id, value) as (select i_id, count(i_id) from bid_history group by i_id) select i_id, if(count_bid.value is NULL, 0, count_bid.value), description, most_recent_bid_price, date_posted, bid_ending_date, buy_it_now_price, if(name is NULL, 'Nobody', name) from count_bid natural right outer join item natural left join (select i_id, bid_price, name from bid_history natural join (select i_id, max(bid_price) as bid_price from User natural join bid_history group by i_id) as temp natural join User) as W where date_posted like \'%%%s%%\' and i_id not in (select i_id from Transaction);", date);
		 
	if (mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	res = mysql_store_result(conn);
	num_row = mysql_num_rows(res);

	item = (int *)malloc(sizeof(int) * num_row);
	printf("----< Search results: Date_posted search >\n");
	i=0;

	while(1){
		row = mysql_fetch_row(res);
	
		if(!row)
			break;

		item[i] = atoi(row[0]);
		printf("[%d]\n", i+1);
		printf("description: %s\n", row[2]);  // description
		printf("status: %s bids\n", row[1]);
		printf("current bidding price: %s\n", row[3]);  // most_recent_bid_price
		printf("current highest bidder: %s\n", row[7]);
		printf("buy-it-now-price: %s\n", row[6]);
		printf("date posted: %s\n", row[4]);  // date_posted
		printf("bid ending date: %s\n", row[5]);  // bid_ending_date
		i++;
	}
	Watched(item, num_row);
	printf("--- Which item do you want to bid? (Enter the number or 'B' to go back to the previous menu) : ");
	scanf("%s", bid);
	if(bid[0] == 'B')
		return;
	
	printf("--- Bidding price? (Enter the price or 'buy' to pay for the buy-it-now price) : ");
	scanf("%s", bid_price);
	bid_history(bid_price, item[atoi(bid)-1]);
	
	free(item);
	free(date);
	free(query);
	free(bid_price);
}


void search_item(){   // search item

	int num;
	printf("----< Search item >\n----(1) Search items by category\n----(2) Search items by description keyword\n----(3) Search items by seller\n----(4) Search items by date posted\n----(5) Go Back\n----(6) Quit\n");
	printf("     Your choice: ");
	scanf("%d", &num);
	getchar();
	switch (num)
	{
	case 1:
		Search_category();
		break;
	case 2:
		Search_keyword();
		break;
	case 3:
		Search_seller();
		break;
	case 4:
		Search_date();
		break;
	case 5:  //Go back
		break;
	case 6:
		exit(0);
	}

}

void sell_item(){     // sell item

	int cate;
	int cond;
	int check=0;
	char * day = (char *)malloc(sizeof(char) *20);
	char * category  = (char *)malloc(sizeof(char) * 20);
	char * condition = (char *)malloc(sizeof(char) * 30);
	char * description = (char *)malloc(sizeof(char) * 50);
	char * query = (char *)malloc(sizeof(char) * 300);
	char buy_it_now_price[8]={0, };

	printf("----< Sell item >\n---- select from the following category\n");
	printf("     (1) Electronics\n     (2) Books\n     (3) Home\n     (4) Clothing\n     (5) Sporting Goods\n     Your choice: ");
	scanf("%d", &cate);
	switch(cate){
		case 1:
			sprintf(category, "Electronics");
			break;
		case 2:
			sprintf(category, "Books");
			break;
		case 3:
			sprintf(category, "Home");
			break;
		case 4:
			sprintf(category, "Clothing");
			break;
		case 5:
			sprintf(category, "Sporting Goods");
	}

	printf("---- condition\n     (1) New\n     (2) Like-New\n     (3) Used (Good)\n     (4) Used (Acceptable)\n     Your choice: ");
	scanf("%d", &cond);
	switch(cond){
		case 1:
			sprintf(condition, "New");
			break;
		case 2:
			sprintf(condition, "Like-New");
			break;
		case 3:
			sprintf(condition, "Used (Good)");
			break;
		case 4:
			sprintf(condition, "Used (Acceptable)");
	}
	getchar();
	printf("---- description: ");
	scanf("%[^\n]s", description);

	while(1){ 
		printf("---- buy-it-now price: ");
		scanf("%s", buy_it_now_price);
		check=0;
		for(int i=0; i<8; i++){

			if(buy_it_now_price[i]== 0)
				break;

			if(!isdigit(buy_it_now_price[i])){
				check = 1;
				break;
			}
		}
		if(check == 0)
			break;
	}
	getchar();
	printf("---- bid_ending date(yyyy-mm-dd HH:mm, e.g.2020-12-04 23:59): ");
	scanf("%[^\n]s", day);
	//date_posted

	sprintf(query, "insert into item(u_id, category, description, conditions, buy_it_now_price, bid_ending_date) values(%d, \"%s\", \"%s\", \"%s\", %s, \"%s\");",u_id, category, description, condition, buy_it_now_price, day);

	if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}
		free(day);
		free(query);
		free(category);
		free(condition);
		free(description);
}



void Status_of_my_item(){

	int i=1;
	char * query = (char *)malloc(sizeof(char) * 600);
	char * query_trans = (char *)malloc(sizeof(char) * 150); 
	printf("----< Status of Your Item Listed on Auction >\n");
	sprintf(query, "with count_bid(i_id, value) as (select i_id, count(i_id) from bid_history group by i_id) select i_id, if(count_bid.value is NULL, 0, count_bid.value), description, most_recent_bid_price, date_posted, bid_ending_date, if(name is NULL, 'Nobody', name) from count_bid natural right outer join item natural left join (select i_id, bid_price, name from bid_history natural join (select i_id, max(bid_price) as bid_price from User natural join bid_history group by i_id) as temp natural join User) as W where item.u_id= %d and i_id not in (select i_id from Transaction);", u_id);
		
	if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}
	res = mysql_store_result(conn);
	
	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[Item %d]\n", i);
		printf("  description: %s\n", row[2]);  // description
		printf("  status: %s bids\n", row[1]);
		printf("  current bidding price: %s\n", row[3]);  // most_recent_bid_price
		printf("  current highest bidder: %s\n", row[6]);
		printf("  date posted: %s\n", row[4]);  // date_posted
		printf("  bid ending date: %s\n", row[5]);  // bid_ending_date
		i++;
	}

	sprintf(query_trans, "select description, sold_price, name, sold_date from Transaction join User join item using(i_id) where buyer = User.u_id and seller = %d;", u_id);
		
	if (mysql_query(conn, query_trans)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}
	res = mysql_store_result(conn);

	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[Item %d]\n", i);
		printf("  description: %s\n", row[0]);  // description
		printf("  status: sold\n");
		printf("  sold price: %s\n", row[1]);  // 
		printf("  buyer: %s\n", row[2]);
		printf("  sold date: %s\n", row[3]);
		i++;
	}
	free(query);
	free(query_trans);
}

void My_account(){

	int i = 1;
	double Commission=0;
	double sold = 0;
	double purchased=0;
	char * query_sold = (char *)malloc(sizeof(char) * 100);
	char * query_buy = (char *)malloc(sizeof(char) * 200);

	printf("----< Check your Account  >\n");
	sprintf(query_sold, "select description, sold_price from Transaction natural join item where seller= %d order by sold_date;", u_id);

	if (mysql_query(conn, query_sold)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}

	res = mysql_store_result(conn);

	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[Sold Item %d]\n", i);
		printf("  description: %s\n", row[0]);  // description
		printf("  sold price: %s\n", row[1]);
		sold += atof(row[1]);
		if(i<=2)
			Commission += atof(row[1]) * 0.02;
		else if(3<=i)
			Commission += atof(row[1]) * 0.01;
		i++;
	}


	sprintf(query_sold, "select description, sold_price  from Transaction natural join item where buyer = %d;", u_id);

	if (mysql_query(conn, query_sold)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}

	res = mysql_store_result(conn);
	i = 1;
	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[Purchased Item %d]\n", i);
		printf("  description: %s\n", row[0]);  // description
		printf("  purchase price: %s\n", row[1]);
		purchased += atof(row[1]);
		i++;
	}

	printf("[Your Balance Summary]\n");
	printf("  sold: %.2f won\n", sold);
	printf("  commission: -%.2f won\n", Commission);
	printf("  purchased: -%.2f won\n", purchased);
	printf("  Total balance: %.2f\n", (sold-Commission-purchased));

	free(query_sold);
	free(query_buy);  
}


void Status_of_my_bid(){

	int i = 1;
	char * query = (char *)malloc(sizeof(char) * 220);
	char * query_trans = (char *)malloc(sizeof(char) * 200); 
	printf("----< Check Status of your Bid >\n");

	sprintf(query, "select description, bid_price, most_recent_bid_price, bid_ending_date from item join (select * from bid_history where u_id = %d and i_id not in (select i_id from Transaction)) as temp using(i_id);", u_id);

	if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}
	res = mysql_store_result(conn);
	
	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[Item %d]\n", i);
		printf("  description: %s\n", row[0]);  // description
		if(!strcmp(row[1], row[2]))
			printf("  status: You are the highest bidder\n");
		else{
			printf("  status: You are out bidded\n");
		}
		printf("  your bidding price: %s\n", row[1]);  // most_recent_bid_price
		printf("  current highest bidding price: %s\n", row[2]);  // date_posted
		printf("  bid ending date: %s\n", row[3]);  // bid_ending_date
		i++;
	}

	sprintf(query_trans, "select description, bid_price, sold_price, sold_date from bid_history join item  using(i_id) natural join Transaction where bid_history.u_id = %d;", u_id);
		
	if (mysql_query(conn, query_trans)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}
	res = mysql_store_result(conn);

	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[Item %d]\n", i);
		printf("  description: %s\n", row[0]);  // description
		if(!strcmp(row[1], row[2]))
			printf("  status: You won the game\n");	
		else{
			printf("  status: You are outbidded and the item is sold.\n");
		}
		printf("  sold price: %s\n", row[2]);  // most_recent_bid_price
		printf("  sold date: %s\n", row[3]);
		i++;
	}

	free(query);
	free(query_trans);
}



void main_page(int a){  //main_page

	switch(a){
		case 1:
			sell_item();
			break;
		case 2:
			Status_of_my_item();
			break;
		case 3:
			search_item();
			break;
		case 4:
			Status_of_my_bid();
			break;
		case 5:
			My_account();
			break;
		case 6:
			exit(0);
	}
}


void show_user(){

	char * query = (char *)malloc(sizeof(char) * 20);
	int i = 1;

	sprintf(query, "select * from User;");

	if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}

	res = mysql_store_result(conn);

	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[User %d]\n", i);
		printf("  name: %s\n", row[1]);  // description
		printf("  email: %s\n", row[2]);  // most_recent_bid_price
		printf("  password: %s\n", row[3]);
		i++;
	}

	free(query);
}


void show_items(){

	char * query = (char *)malloc(sizeof(char) * 600);
	char * trans_query = (char *)malloc(sizeof(char) * 150);
	int i = 1;

	sprintf(query, "with count_bid(i_id, value) as (select i_id, count(i_id) from bid_history group by i_id) select category, description, conditions, if(count_bid.value is NULL, 0, count_bid.value), most_recent_bid_price, if(name is NULL, 'Nobody', name), buy_it_now_price, date_posted, bid_ending_date from count_bid natural right outer join item natural left join (select i_id, bid_price, name from bid_history natural join (select i_id, max(bid_price) as bid_price from User natural join bid_history group by i_id) as temp natural join User) as W where i_id not in (select i_id from Transaction);");

	if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}

	res = mysql_store_result(conn);
	
	
	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[item %d]\n", i);
		printf("  Category: %s\n", row[0]);
		printf("  description: %s\n", row[1]);
		printf("  condition: %s\n", row[2]);
		printf("  status: %s bids\n", row[3]);
		printf("  current bidding price: %s\n", row[4]);
		printf("  current highest bidder: %s\n", row[5]);
		printf("  buy-it-now price: %s\n", row[6]);
		printf("  date posted: %s\n", row[7]);
		printf("  bid ending date: %s\n", row[8]);
		i++;
	}

	sprintf(trans_query, "select category, description, conditions, sold_price, name, sold_date from Transaction join User join item using(i_id) where buyer = User.u_id;");

	if (mysql_query(conn, trans_query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}

	res = mysql_store_result(conn);

	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("[item %d]\n", i);
		printf("  Category: %s\n", row[0]);
		printf("  description: %s\n", row[1]);
		printf("  condition: %s\n", row[2]);
		printf("  status: sold\n");
		printf("  sold price: %s\n", row[3]);
		printf("  buyer: %s\n", row[4]);
		printf("  sold date: %s\n", row[5]);
		i++;
	}

	free(trans_query);
	free(query);
}


void show_Transaction(){

	char * query =(char *)malloc(sizeof(char) * 90);

	sprintf(query, "select category, count(i_id) from item natural join Transaction group by category");
	

	if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}

	res = mysql_store_result(conn);

	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("  Category: %s\n", row[0]);
		printf("  Number of Transaction: %s\n", row[1]);
	}
	free(query);
}

void show_Profit(){
	char * query = (char *)malloc(sizeof(char) * 100);
	int num[100]={0, };
	double profit = 0;
	sprintf(query, "select seller, sold_price, sold_date from Transaction order by sold_date asc");

	if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}

	res = mysql_store_result(conn);

	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		if(num[atoi(row[0])] < 2){
			num[atoi(row[0])] ++;
			profit += atof(row[1]) * 0.02;
		}
		else{
			num[atoi(row[0])] ++;
			profit += atof(row[1]) * 0.01;
		}
	}

	printf("profit of our company: %.2f \n", profit);

	free(query);
}


void rank_item(){

	char * query = (char *)malloc(sizeof(char)*160);
	int i = 1;
	sprintf(query, "select category, description, sold_price, name, sold_date from Transaction join User join item using(i_id) where buyer = User.u_id order by sold_price desc;");
	

	if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
	}

	res = mysql_store_result(conn);

	while(1){
		row = mysql_fetch_row(res);
		if(!row)
			break;
		printf("Rank %d\n", i);
		printf("  Category: %s\n", row[0]);
		printf("  description: %s\n", row[1]);
		printf("  sold price: %s\n", row[2]);
		printf("  buyer: %s\n", row[3]);
		printf("  sold date: %s\n", row[4]);
		i++;
	}

	free(query);

}


void admin_page(){

	int num;

	while(1){
		printf("----< admin_Menu >\n");
		printf("----(1) Show User \n");
		printf("----(2) Show items \n");
		printf("----(3) Transaction per category\n");
		printf("----(4) Profits of company\n");
		printf("----(5) Rank of item\n");
		printf("----(6) Quit\n");
		printf("select number: ");
		scanf("%d", &num);

		switch(num){
			case 1:
				show_user();
				break;
			case 2:
				show_items();
				break;
			case 3:
				show_Transaction();
				break;
			case 4:
				show_Profit();
				break;
			case 5:
				rank_item();
				break;
			case 6:
				exit(0);
		}
	}
}


void Register(){
	char * name = (char *)malloc(sizeof(char) * 30);
	char * last_name = (char *)malloc(sizeof(char) * 15);
	char * email = (char *)malloc(sizeof(char) * 45);
	char * password = (char *)malloc(sizeof(char) * 30);
	char * query = (char *)malloc(sizeof(char) * 300);

		printf("----< Sign up >\n---- fisrt name: ");
		scanf("%s", name); //first name
		printf("---- last name: ");
		scanf("%s", last_name);
		printf("---- email: ");
		scanf("%s", email);
		printf("---- password: ");
		scanf("%s", password);

		strcat(name, last_name); 
		sprintf(query, "insert into User(name, email, password) values(\"%s\", \"%s\", \"%s\");", name, email, password);

		if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}

	free(name);
	free(last_name);
	free(email);
	free(password);
	free(query);
}

void Login(){

	char * email = (char *)malloc(sizeof(char) * 45);
	char * password = (char *)malloc(sizeof(char) * 30);
	char * query = (char *)malloc(sizeof(char) * 300);
	int choice;

		printf("----< Login >\n");
		printf("---- email: ");	
		scanf("%s", email);
		printf("---- password: ");
		scanf("%s", password);
		sprintf(query, "select u_id, email, password from User where email=\"%s\" and password=\"%s\"", email, password);
		
		if (mysql_query(conn, query)) {
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}

		res = mysql_use_result(conn);

		row = mysql_fetch_row(res);

		if(!row){
			printf("---- Not registered\n");
		}
		else{
			u_id = atoi(row[0]);
			printf("%d\n", u_id);
			while(1){
				printf("----< Main menu >\n----(1) Sell item\n----(2) Status of Your Item Listed in Auction\n----(3) Search item\n----(4) Check Status of your Bid\n----(5) Check your Account\n----(6) Quit\n");
				printf("    Your choice: ");
				scanf("%d", &choice);
				if(choice == 6){
					exit(0);
				}

				mysql_free_result(res);
				main_page(choice);     //goto main_page
			}
		}
	free(email);
	free(password);
	free(query);
}


void first_page(int a){    //first_page

	char * email = (char *)malloc(sizeof(char) * 45);
	char * password = (char *)malloc(sizeof(char) * 30);
	char * query = (char *)malloc(sizeof(char) * 300);

	if(a == 1){
		Login();
	}
	else if(a == 2){
		Register();
	}
	else if( a == 3){
		printf("----< Login as Administrator  >\n---- email :");
		scanf("%s", email);
		printf("---- password: ");
		scanf("%s", password);
		if(!strcmp(email, "mysql") && !strcmp(password, "mongodb")){
			printf("---- Hello admisnitrator\n");
			admin_page();
		}
		else{
			printf("---- Your not admin\n");
		}
	}
	free(email);
	free(password);
	free(query);
}

int main() {    
	
	/* set database information*/
	char *server = "localhost";
	char *user = "db16314167";
	char *password = "yang7769!!"; 
	char *database = "db16314167";

	conn = mysql_init(NULL);
	int num;
	/* Connect to database */
	if (!mysql_real_connect(conn, server, user, password, 
                                      database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}
	

	/*
	char * trigger_transaction = "create trigger transaction_buy after update on item for each row begin if new.most_recent_bid_price >= new.buy_it_now_price then insert into Transaction(i_id, seller, sold_date, sold_price) values(new.i_id, new.u_id, now(), new.most_recent_bid_price); end if; end;";
	//char * trigger_transaction = "DELIMITER //  \n create trigger transaction_bid_end after insert on item for each row begin var if new.most_recent_bid_price = buy_it_now_price then insert into Transaction(i_id, seller, sold_date, sold_price) values(new.i_id, new.u_id, new(), new.most_recent_bid_price); end if; end; // DELIMITER ;";
	char * trigger_bid = "create trigger bid_money before update on item for each row begin if new.most_recent_bid_price < (select * from (select most_recent_bid_price from item where i_id=new.i_id) as W) then SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'Warning: you have to bid more money';end if; end;";

	if (mysql_query(conn, trigger_transaction)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}
	*//*
	if (mysql_query(conn, trigger_bid)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}
	*/
	/* send SQL query */
	
	/*
	char * sql_user = "create table User(u_id	int AUTO_INCREMENT, name varchar(30), email	varchar(45) NOT NULL UNIQUE, password varchar(30), primary key (u_id))";
	
	char * sql_item = "create table item(i_id	int AUTO_INCREMENT, u_id int, category varchar(20) check (category in ('Electronics', 'Books', 'Home', 'Clothing', 'Sporting Goods')), description varchar(50), conditions varchar(30) check (conditions in ('New', 'Like-New', 'Used (Good)', 'Used (Acceptable)')), buy_it_now_price numeric(8) , date_posted DATETIME not null default now(), bid_ending_date DATETIME, most_recent_bid_price numeric(8) default '0', primary key (i_id), foreign key (u_id) references User(u_id) on delete cascade)";

	char * sql_transaction = "create table Transaction(transaction_id	int AUTO_INCREMENT, i_id	int, seller int, buyer int, sold_date DATETIME default now(), sold_price numeric(8), primary key (transaction_id), foreign key (i_id) references item(i_id) on delete set null, foreign key (seller) references User(u_id) on delete set null, foreign key (buyer) references User(u_id)	on delete set null)";

	char * sql_Watch = "create table Watch_list( u_id	int, i_id	int, primary key (u_id, i_id), foreign key (u_id) references User(u_id) on delete cascade, foreign key (i_id) references item(i_id) on delete cascade)";
	
	char * sql_bid = "create table bid_history(u_id	int, i_id int, bid_price numeric(8,0), primary key (i_id, bid_price), foreign key (u_id) references User(u_id) on delete cascade, foreign key (i_id) references item(i_id)on delete cascade)";

	

	if (mysql_query(conn, sql_user)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}
	
	if (mysql_query(conn, sql_item)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	if (mysql_query(conn, sql_transaction)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	if (mysql_query(conn, sql_Watch)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	if (mysql_query(conn, sql_bid)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}
	
	*/
	
	bid_ending();

	while(1){
		printf("----(1) Login\n----(2) Sign Up\n----(3) Login as Administrator (Manager of Auction System)\n----(4) Quit\n");
		printf("    Your choice: ");
		scanf("%d", &num);
		if( num == 4 )
			break;
		first_page(num);
	}

	/* output table name */

	/* close connection */
	mysql_free_result(res);
	mysql_close(conn);
}

