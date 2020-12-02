create table User(u_id	int AUTO_INCREMENT, name varchar(30), email	varchar(45) NOT NULL UNIQUE, password varchar(30), primary key (u_id));

create table item(i_id	int AUTO_INCREMENT, u_id int, category varchar(20) check (category in ('Electronics', 'Books', 'Home', 'Clothing', 'Sporting Goods')), description varchar(50), conditions varchar(30) check (conditions in ('New', 'Like-New', 'Used (Good)', 'Used (Acceptable)')), buy_it_now_price numeric(8) , date_posted DATETIME not null default now(), bid_ending_date DATETIME, most_recent_bid_price numeric(8) default '0', primary key (i_id), foreign key (u_id) references User(u_id) on delete cascade);

create table Transaction(transaction_id	int AUTO_INCREMENT, i_id	int, seller int, buyer int, sold_date DATETIME default now(), sold_price numeric(8), primary key (transaction_id), foreign key (i_id) references item(i_id) on delete set null, foreign key (seller) references User(u_id) on delete set null, foreign key (buyer) references User(u_id)	on delete set null);

create table Watch_list( u_id	int, i_id	int, primary key (u_id, i_id), foreign key (u_id) references User(u_id) on delete cascade, foreign key (i_id) references item(i_id) on delete cascade);

create table bid_history(u_id	int, i_id int, bid_price numeric(8,0), primary key (i_id, bid_price), foreign key (u_id) references User(u_id) on delete cascade, foreign key (i_id) references item(i_id)on delete cascade);



DELIMITER // 
create trigger transaction_buy after update on item 
for each row 
begin 
if new.most_recent_bid_price >= new.buy_it_now_price 
then insert into Transaction(i_id, seller, sold_date, sold_price) values(new.i_id, new.u_id, now(), new.most_recent_bid_price); 
end if; 
end;// 
DELIMITER ;

DELIMITER // 
create trigger bid_money before update on item 
for each row 
begin 
if new.most_recent_bid_price < (select * from (select most_recent_bid_price from item where i_id=new.i_id) as W) 
then SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'Warning: you have to bid more money';
end if;
end;// 
DELIMITER ;