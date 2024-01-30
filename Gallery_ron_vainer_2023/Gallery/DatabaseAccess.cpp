#include "DatabaseAccess.h"
#include <io.h>



bool DatabaseAccess::open()
{
    this->_dbName = "GalleryDB.sqlite";
    int file_exist = _access(this->_dbName.c_str(), 0);
    int res = sqlite3_open(this->_dbName.c_str(), &this->_db);
    this->_res = res;
    if (res != SQLITE_OK) {
        this->_db = nullptr;
        std::cerr << "Failed to open DB" << std::endl;
        return false;
    }
    if (file_exist != 0) {
        //init database
        char** errMessage = nullptr;
        const char* query = "CREATE TABLE USERS ( ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,NAME text NOT NULL);";
            
        res = sqlite3_exec(this->_db, query, nullptr, nullptr, errMessage);
        if (res != SQLITE_OK) {
            return false;
        }
        
        query = "CREATE TABLE ALBUMS( ID INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT  NULL, CREATION_DATE DATE NOT  NULL,USER_ID INTEGER  NOT NULL, FOREIGN KEY(USER_ID) REFERENCES USERS(ID));";
        res = sqlite3_exec(this->_db, query, nullptr, nullptr, errMessage);
        if (res != SQLITE_OK) {
            return false;
        }
        query = "CREATE TABLE PICTURES (ID INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT  NULL,LOCATION TEXT NOT NULL,CREATION_DATE DATE NOT  NULL,ALBUM_ID INTEGER  NOT NULL,FOREIGN KEY(ALBUM_ID) REFERENCES ALBUMS(ID) );";
        res = sqlite3_exec(this->_db, query, nullptr, nullptr, errMessage);
        if (res != SQLITE_OK) {
            return false;
        }
        query = "CREATE TABLE TAGS (ID INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,PICTURE_ID INTEGER NOT NULL,USER_ID INTEGER NOT NULL,FOREIGN KEY(PICTURE_ID) REFERENCES PICTURES(ID)FOREIGN KEY(USER_ID) REFERENCES USERS(ID));";
        res = sqlite3_exec(this->_db, query, nullptr, nullptr, errMessage);
        if (res != SQLITE_OK) {
            return false;
        }

       


    }
    
}

void DatabaseAccess::close()
{
    sqlite3_close(this->_db);
    this->_db = nullptr;
}

void DatabaseAccess::clear()
{
    close();
    this->_db = nullptr;
    this->_dbName = "";
    this->_res = 0;
    
}

int callbackGetAlbums(void* data, int argc, char** argv, char** azColName) {


    std::list<Album>* albums = (std::list<Album>*)(data);
    Album album(atoi(argv[3]), argv[1], argv[2]);
    albums->push_back(album);
    return 0;
}

const std::list<Album> DatabaseAccess::getAlbums()
{
    std::string qe = "SELECT * FROM ALBUMS;";
    std::list<Album> albums;
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetAlbums, &albums, errMessage);
    return albums;
}

const std::list<Album> DatabaseAccess::getAlbumsOfUser(const User& user)
{
    std::string qe = "SELECT * FROM ALBUMS INNER JOIN USERS ON USERS.ID == ALBUMS.USER_ID WHERE USERS.ID == " + std::to_string(user.getId()) + ";";
    std::list<Album> albums;
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetAlbums, &albums, errMessage);
    return albums;
}

void DatabaseAccess::createAlbum(const Album& album)
{
    std::string qe = "INSERT INTO ALBUMS(NAME, USER_ID, CREATION_DATE) VALUES(\"" + album.getName() + "\"," + std::to_string(album.getOwnerId()) + ",\"" + album.getCreationDate() + "\");";
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), nullptr, nullptr, errMessage);
    if (this->_res != SQLITE_OK) {
        return;

    }
}

void DatabaseAccess::deleteAlbum(const std::string& albumName, int userId)
{
    std::string qe = "BEGIN TRANSACTION; ";
    qe += "DELETE FROM TAGS WHERE PICTURE_ID IN (SELECT ID FROM PICTURES WHERE ALBUM_ID = (SELECT ID FROM ALBUMS WHERE NAME == \"" + albumName + "\" AND ALBUMS.USER_ID == " + std::to_string(userId) + ")); ";
    qe += "DELETE FROM PICTURES WHERE PICTURES.ALBUM_ID IN (SELECT ID FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\" AND ALBUMS.USER_ID == " + std::to_string(userId) + ");";
    qe += "DELETE FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\" AND ALBUMS.USER_ID == " + std::to_string(userId) + "; COMMIT;";
    char** errMessage = nullptr;
    const char* query = qe.c_str();
    this->_res = sqlite3_exec(this->_db, query, nullptr, nullptr, errMessage);
    if (this->_res != SQLITE_OK) {
     return;

    }

    

}

int callbackDoesExist(void* data, int argc, char** argv, char** azColName)
{
    bool* exist = (bool*)(data);
    *exist = (argc > 1);



    return 0;
}

bool DatabaseAccess::doesAlbumExists(const std::string& albumName, int userId)
{
    std::string qe = "SELECT * FROM ALBUMS WHERE ALBUMS.USER_ID == " + std::to_string(userId) + " AND  ALBUMS.NAME == \"" + albumName + "\";";;
    char** errMessage = nullptr;
    const char* query = qe.c_str();
    bool exist = false;
    this->_res = sqlite3_exec(this->_db, query, callbackDoesExist, &exist, errMessage);
    if (this->_res != SQLITE_OK) {

        return false;

    }
    return exist;
    
}

Album DatabaseAccess::openAlbum(const std::string& albumName)
{
    std::string qe = "SELECT * FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\"";
    char** errMessage = nullptr;
    std::list<Album> albums;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetAlbums, &albums, errMessage);
    Album album(albums.front());

    std::vector<Picture> pictures = getPictures(albumName);

    for (auto& it : pictures) {
        std::vector<int> tags = getTagsOfPicture(albumName, it.getName());
        for (auto& tag : tags) {
            it.tagUser(tag);
        }
    }


    for (auto& it : pictures) {
        album.addPicture(it);
    }

    return album;
}

void DatabaseAccess::closeAlbum(Album& pAlbum)
{
}

void DatabaseAccess::printAlbums()
{
    std::list<Album> albums = getAlbums();
    for (auto& it : albums) {
        std::cout << it << std::endl;
    }
}

int callbackGetPictures(void* data, int argc, char** argv, char** azColName) {
    std::vector<Picture>* pictures = (std::vector<Picture>*)(data);
    
    pictures->push_back(Picture(atoi(argv[0]), argv[1], argv[2], argv[3]));

    return 0;
}

std::vector<Picture> DatabaseAccess::getPictures(const std::string& albumName)
{
    std::string qe = "SELECT * FROM PICTURES WHERE PICTURES.ALBUM_ID == (SELECT ALBUMS.ID FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\");";
    std::vector<Picture> pictures;
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetPictures, &pictures, errMessage);
    
    return pictures;
}

int callbackGetTags(void* data, int argc, char** argv, char** azColName) {
    std::vector<int>* tags = (std::vector<int>*)(data);
    tags->push_back(atoi(argv[2]));
    return 0;
}

std::vector<int> DatabaseAccess::getTagsOfPicture(const std::string albumName, const std::string pictureName)
{
    std::string qe = "SELECT * FROM TAGS WHERE TAGS.PICTURE_ID == (SELECT PICTURES.ID FROM PICTURES INNER JOIN ALBUMS ON ALBUMS.ID == PICTURES.ALBUM_ID WHERE ALBUMS.NAME == \"" + albumName + "\" AND PICTURES.NAME == \"" + pictureName + "\");";
    char** errMessage = nullptr;
    
    std::vector<int> tags;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetTags, &tags, errMessage);

    return tags;
}

void DatabaseAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture)
{
    
    std::string qe = "INSERT INTO PICTURES(ID, NAME, LOCATION, CREATION_DATE, ALBUM_ID) VALUES( " + std::to_string(picture.getId()) + ",\"" + picture.getName() + "\",\"" + picture.getPath() + "\",\"" + picture.getCreationDate() + "\", (SELECT ALBUMS.ID FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\"));";

    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), nullptr, nullptr, errMessage);
    if (this->_res != SQLITE_OK) {

        return;

    }
}

void DatabaseAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName)
{
    std::string qe = "BEGIN TRANSACTION; ";
    qe += "DELETE FROM TAGS WHERE TAGS.PICTURE_ID == (SELECT PICTURES.ID FROM PICTURES WHERE PICTURES.NAME == \"" + pictureName + "\"); COMMIT;";
    qe += "DELETE FROM PICTURES WHERE PICTURES.ALBUM_ID == (SELECT ALBUMS.ID FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\") AND PICTURES.NAME == \"" + pictureName + "\";";
    
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), nullptr, nullptr, errMessage);
    if (this->_res != SQLITE_OK) {

        return;
    }

}

void DatabaseAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
    std::string qe = "INSERT INTO TAGS (PICTURE_ID, USER_ID) VALUES((SELECT PICTURES.ID FROM PICTURES WHERE PICTURES.NAME == \"" + pictureName + "\"" + " AND PICTURES.ALBUM_ID == (SELECT ALBUMS.ID FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\"))," + std::to_string(userId)  + ");";
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), nullptr, nullptr, errMessage);
    if (this->_res != SQLITE_OK) {

        return;
    }
}

void DatabaseAccess::untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
    std::string qe = "DELETE FROM TAGS WHERE TAGS.PICTURE_ID == (SELECT PICTURES.ID FROM PICTURES WHERE PICTURES.NAME == \"" + pictureName + "\") AND TAGS.USER_ID == " + std::to_string(userId) + ";";
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), nullptr, nullptr, errMessage);
    if (this->_res != SQLITE_OK) {

        return;
    }

}
int callbackGetUsers(void* data, int argc, char** argv, char** azColName) {

    
    std::vector<User>* users = (std::vector<User>*)(data);
     User us(atoi(argv[0]), argv[1]);
    users->push_back(us);
    return 0;
}


std::vector<User> DatabaseAccess::getUsers() 
{
    std::string qe = "SELECT * FROM USERS;";
    std::vector<User> users;
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetUsers, &users, errMessage);
    return users;
}

int DatabaseAccess::getLastIdOfPicture()
{
    std::vector<Picture> pictures = getAllPictures();
    if (pictures.size() == 0) {
        return 100;
    }
    return pictures.back().getId();
}

std::vector<Picture> DatabaseAccess::getAllPictures()
{
    std::string qe = "SELECT * FROM PICTURES;";
    char** errMessage = nullptr;
    std::vector<Picture> pictures;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetPictures, &pictures, errMessage);
    return pictures;

}

int DatabaseAccess::getLastIdOfUser() 
{
    std::vector<User> users = getUsers();
    if (users.size() == 0) {
        return 200;
    }
    return users.back().getId();
}




void DatabaseAccess::printUsers()
{
    std::vector<User> users = getUsers();
    for (auto& it : users) {
        std::cout << it << std::endl;
    }
    
}



void DatabaseAccess::createUser(User& user)
{
    std::string qe = "INSERT INTO USERS(ID, NAME) VALUES( " + std::to_string(user.getId()) + ",\"" + user.getName() + "\"); ";
    const char* query = qe.c_str();
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, query, nullptr, nullptr, errMessage);
    if (this->_res != SQLITE_OK) {
        return;

    }
}


/*
* std::string qe = "BEGIN TRANSACTION; ";
    qe += "DELETE FROM TAGS WHERE PICTURE_ID IN (SELECT ID FROM PICTURES WHERE ALBUM_ID = (SELECT ID FROM ALBUMS WHERE NAME == \"" + albumName + "\" AND ALBUMS.USER_ID == " + std::to_string(userId) + ")); ";
    qe += "DELETE FROM PICTURES WHERE PICTURES.ALBUM_ID IN (SELECT ID FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\" AND ALBUMS.USER_ID == " + std::to_string(userId) + "); ";
    qe += "DELETE FROM ALBUMS WHERE ALBUMS.NAME == \"" + albumName + "\" AND ALBUMS.USER_ID == " + std::to_string(userId) + "; COMMIT;";
*/

void DatabaseAccess::deleteUser(const User& user)
{
    std::string qe = "BEGIN TRANSACTION; ";
    qe += "DELETE FROM TAGS WHERE TAGS.USER_ID == " + std::to_string(user.getId()) + ";";
    qe += "DELETE FROM PICTURES WHERE PICTURES.ALBUM_ID IN (SELECT ID FROM ALBUMS WHERE ALBUMS.USER_ID == " + std::to_string(user.getId()) + "); ";
    qe += "DELETE FROM USERS WHERE USERS.ID == " + std::to_string(user.getId()) + "; ";
    qe += "DELETE FROM ALBUMS WHERE ALBUMS.USER_ID == " + std::to_string(user.getId()) + "; ";
    qe += "COMMIT;";
    
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), nullptr, nullptr, errMessage);
    if (this->_res != SQLITE_OK) {

        return;

    }
}




bool DatabaseAccess::doesUserExists(int userId)
{
    std::string qe = "SELECT * FROM USERS WHERE USERS.ID == " + std::to_string(userId) + ";";
     char** errMessage = nullptr;
     const char* query = qe.c_str();
     bool exist = false;
     this->_res = sqlite3_exec(this->_db, query, callbackDoesExist, &exist, errMessage);
     if (this->_res != SQLITE_OK) {
         
         return false;

     }
    return exist;
}

int callbackGetUser(void* data, int argc, char** argv, char** azColName)
{
    User* us= (User*)data;
    us->setId(atoi(argv[0]));
    
    us->setName(argv[1]);
    
    return 0;
}

User DatabaseAccess::getUser(int userId)
{
    std::string qe = "SELECT * FROM USERS WHERE USERS.ID == " + std::to_string(userId) + ";";
    char** errMessage = nullptr;
    User user(0, "0");
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetUser, &user, errMessage);
    if (this->_res != SQLITE_OK) {

        throw(std::exception());

    }
    return user;
}

int callbackGetInteger(void* data, int argc, char** argv, char** azColName) {
    int* count = (int*)(data);
    *count = atoi(argv[0]);
    
    return *count;
}

int DatabaseAccess::countAlbumsOwnedOfUser(const User& user)
{
    std::string qe = "SELECT COUNT(*) FROM ALBUMS WHERE ALBUMS.USER_ID == " + std::to_string(user.getId()) + ";";
    char** errMessage = nullptr;
    int count = 0;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetInteger, &count, errMessage);
    
    return count;
}

int DatabaseAccess::countAlbumsTaggedOfUser(const User& user)
{
    std::string qe = "SELECT COUNT(DISTINCT ALBUMS.ID) FROM PICTURES JOIN TAGS ON PICTURES.ID == TAGS.PICTURE_ID JOIN ALBUMS ON PICTURES.ALBUM_ID == ALBUMS.ID WHERE TAGS.USER_ID == " + std::to_string(user.getId())  + " GROUP BY ALBUMS.USER_ID;";
    char** errMessage = nullptr;
    int count = 0;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetInteger, &count, errMessage);

    return count;
}

int DatabaseAccess::countTagsOfUser(const User& user)
{
    std::string qe = "SELECT COUNT(*) FROM TAGS WHERE TAGS.USER_ID == " + std::to_string(user.getId()) + ";";
    char** errMessage = nullptr;
    int count = 0;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetInteger, &count, errMessage);
    
    return count;
}

float DatabaseAccess::averageTagsPerAlbumOfUser(const User& user)
{
    return countTagsOfUser(user) / float(countAlbumsTaggedOfUser(user));
}





User DatabaseAccess::getTopTaggedUser()
{
    std::string qe = "SELECT TAGS.USER_ID, USERS.NAME FROM TAGS INNER JOIN USERS ON USERS.ID == TAGS.USER_ID GROUP BY TAGS.USER_ID ORDER BY COUNT(*) DESC LIMIT 1;";
    char** errMessage = nullptr;
    User user(0, "0");
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetUser, &user, errMessage);
    if (this->_res != SQLITE_OK) {

        throw(std::exception());

    }
    if (user.getId() == 0) {
        throw(std::exception("There are no tags"));
    }
    return user;
}
int callbackGetPicture(void* data, int argc, char** argv, char** azColName) {
    Picture* pic = (Picture*)(data);
    pic->setName(argv[1]);
    
    pic->setId(atoi(argv[0]));
    return 0;
}


Picture DatabaseAccess::getTopTaggedPicture()
{
    std::string qe = "SELECT TAGS.PICTURE_ID, PICTURES.NAME FROM TAGS INNER JOIN PICTURES ON PICTURES.ID == TAGS.PICTURE_ID GROUP BY TAGS.PICTURE_ID ORDER BY COUNT(*) DESC LIMIT 1;";
    char** errMessage = nullptr;
    Picture picture(0, "0");
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetPicture, &picture, errMessage);
    if (this->_res != SQLITE_OK) {

        throw(std::exception());

    }
    if (picture.getId() == 0) {
        throw(std::exception("There are no tags"));
    }
    

    return picture;
}

int callbackGetPictureList(void* data, int argc, char** argv, char** azColName) {
    std::list<Picture>* pics = (std::list< Picture>*)(data);
    pics->push_back(Picture(atoi(argv[0]), argv[1], argv[2], argv[3]));
    return 0;
}


std::list<Picture> DatabaseAccess::getTaggedPicturesOfUser(const User& user)
{
    std::list<Picture> pics;
    std::string qe = "SELECT * FROM PICTURES INNER JOIN TAGS ON TAGS.PICTURE_ID == PICTURES.ID WHERE TAGS.USER_ID == " + std::to_string(user.getId()) + ";";
    char** errMessage = nullptr;
    this->_res = sqlite3_exec(this->_db, qe.c_str(), callbackGetPictureList, &pics, errMessage);
    if (this->_res != SQLITE_OK) {

        throw(std::exception());

    }
    return pics;
}










