DROP DATABASE IF EXISTS xbook_db;
DROP FUNCTION IF EXISTS delete_book_history();
DROP FUNCTION IF EXISTS delete_cd_history();
DROP FUNCTION IF EXISTS delete_dvd_history();
DROP FUNCTION IF EXISTS delete_journal_history();
DROP FUNCTION IF EXISTS delete_magazine_history();
DROP FUNCTION IF EXISTS delete_request();
DROP FUNCTION IF EXISTS delete_videogame_history();
DROP TABLE IF EXISTS admin;
DROP TABLE IF EXISTS book CASCADE;
DROP TABLE IF EXISTS book_binding_types;
DROP TABLE IF EXISTS book_copy_info;
DROP TABLE IF EXISTS book_files;
DROP TABLE IF EXISTS cd CASCADE;
DROP TABLE IF EXISTS cd_copy_info;
DROP TABLE IF EXISTS cd_formats;
DROP TABLE IF EXISTS cd_songs;
DROP TABLE IF EXISTS dvd CASCADE;
DROP TABLE IF EXISTS dvd_aspect_ratios;
DROP TABLE IF EXISTS dvd_copy_info;
DROP TABLE IF EXISTS dvd_ratings;
DROP TABLE IF EXISTS dvd_regions;
DROP TABLE IF EXISTS grey_literature;
DROP TABLE IF EXISTS item_borrower CASCADE;
DROP TABLE IF EXISTS item_request;
DROP TABLE IF EXISTS journal CASCADE;
DROP TABLE IF EXISTS journal_copy_info;
DROP TABLE IF EXISTS journal_files;
DROP TABLE IF EXISTS languages;
DROP TABLE IF EXISTS locations;
DROP TABLE IF EXISTS magazine CASCADE;
DROP TABLE IF EXISTS magazine_copy_info;
DROP TABLE IF EXISTS magazine_files;
DROP TABLE IF EXISTS member;
DROP TABLE IF EXISTS member_history;
DROP TABLE IF EXISTS member_history_dnt;
DROP TABLE IF EXISTS minimum_days;
DROP TABLE IF EXISTS monetary_units;
DROP TABLE IF EXISTS photograph;
DROP TABLE IF EXISTS photograph_collection;
DROP TABLE IF EXISTS videogame CASCADE;
DROP TABLE IF EXISTS videogame_copy_info;
DROP TABLE IF EXISTS videogame_platforms;
DROP TABLE IF EXISTS videogame_ratings;
DROP ROLE IF EXISTS biblioteq_administrator;
DROP ROLE IF EXISTS biblioteq_circulation;
DROP ROLE IF EXISTS biblioteq_circulation_librarian;
DROP ROLE IF EXISTS biblioteq_circulation_librarian_membership;
DROP ROLE IF EXISTS biblioteq_circulation_membership;
DROP ROLE IF EXISTS biblioteq_guest;
DROP ROLE IF EXISTS biblioteq_librarian;
DROP ROLE IF EXISTS biblioteq_librarian_membership;
DROP ROLE IF EXISTS biblioteq_membership;
DROP ROLE IF EXISTS biblioteq_patron;
DROP USER IF EXISTS xbook_admin;
DROP USER IF EXISTS xbook_guest;
