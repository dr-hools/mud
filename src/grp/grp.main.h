#ifndef BYLINS_GRP_MAIN_H
#define BYLINS_GRP_MAIN_H

#include <utility>
#include "chars/char.hpp"

enum RQ_TYPE {RQ_GROUP, RQ_PERSON, RQ_ANY};
enum GRP_COMM {GRP_COMM_LEADER, GRP_COMM_ALL, GRP_COMM_OTHER};
enum RQ_R {RQ_R_OK, RQ_R_NO_GROUP, RQ_R_OVERFLOW, RQ_REFRESH};
enum INV_R {INV_R_OK, INV_R_NO_PERSON, INV_R_BUSY, INV_R_REFRESH};

void do_grequest(CHAR_DATA *ch, char *argument, int, int);
void change_leader(CHAR_DATA *ch, CHAR_DATA *vict);
void do_group(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_group2(CHAR_DATA *ch, char *argument, int, int);
void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);
int max_group_size(CHAR_DATA *ch);
bool isGroupedFollower(CHAR_DATA* master, CHAR_DATA* vict);

class Group;
class Request;

using namespace std::chrono;
using grp_ptr = std::shared_ptr<Group>;
using rq_ptr = std::shared_ptr<Request>;
using sclock_t = time_point<std::chrono::steady_clock>;

const duration DEF_EXPIRY_TIME = 600s;

struct char_info {
    char_info(int memberUid, CHAR_DATA *member, const std::string& memberName);
    virtual ~char_info();
    int memberUID; // часть ключа
    CHAR_DATA * member; // ссылка на персонажа, может быть невалидна!
    std::string memberName; // бэкап имени, если персонаж оффлайн
    sclock_t expiryTime; // время, когда запись автоматом удаляется после проверок.
};

class Group {
private:
    // ид группы в ростере
    u_long _uid = 0;
    //макс.количество игроков
    int _memberCap = 0;
    // ид лидера
    int _leaderUID;
    // имя лидера
    std::string _leaderName;
    // ссылка на персонажа, АХТУНГ! Может меняться и быть невалидным
    CHAR_DATA* _leader = nullptr;
public:
    u_long getUid() const;
    u_short getCurrentMemberCount() const;
    const std::string &getLeaderName() const;
    CHAR_DATA *getLeader() const;
    void _clear(bool silent);
    Group(CHAR_DATA *leader, u_long uid);
    ~Group();
    void _setLeader(CHAR_DATA *leader);
    int _getMemberCap() const;
    bool _isFull();
    bool _isActive(); // проверка, что в группе все персонажи онлайн
    bool _isMember(int uid);
    const char* _getMemberName(int uid);
    int _findMember(char* memberName);
    CHAR_DATA* _findMember(int UID);
    bool _removeMember(CHAR_DATA *member);
    void charDataPurged(CHAR_DATA* ch);
    u_short size() { return (u_short)_memberList->size();}
private:
    std::map<int, std::shared_ptr<char_info *>> * _memberList;
    static void _printHeader(CHAR_DATA* ch, bool npc);
    static void _printDeadLine(CHAR_DATA* ch, const char* playerName, int header);
    static void _printNPCLine(CHAR_DATA* ch, CHAR_DATA* npc, int header);
    static void _printPCLine(CHAR_DATA* ch, CHAR_DATA* pc, int header);
    bool _sameGroup(CHAR_DATA * ch, CHAR_DATA * vict);
public:
    void addFollowers(CHAR_DATA* leader);
    void addMember(CHAR_DATA *member);
    void expellMember(char* memberName);
    bool _restoreMember(CHAR_DATA *member);

    void printGroup(CHAR_DATA *requestor);
    void listMembers(CHAR_DATA *requestor);

    void promote(char *applicant);
    void approveRequest(const char *applicant);
    void rejectRequest(char *applicant);
    void leaveGroup(CHAR_DATA* vict);

    void sendToGroup(GRP_COMM mode, const char *msg, ...);
    void actToGroup(CHAR_DATA* vict, GRP_COMM mode, const char *msg, ...);
public:
    // всякий унаследованный стафф
    CHAR_DATA* get_random_pc_group();
};

class Request {
public:
    sclock_t _time;
    CHAR_DATA *_applicant;
    Group* _group;
    std::string _applicantName;
    int _applicantUID;
    RQ_TYPE _type;
    Request(CHAR_DATA* subject, Group* group, RQ_TYPE type);
};


class GroupRoster {
// properties
public:
    GroupRoster();
    void restorePlayerGroup(CHAR_DATA* ch); // возвращает игрока в группу после смерти
    void processGroupCommands(CHAR_DATA *ch, char *argument);
    void printList(CHAR_DATA *ch);
private:
    u_long _currentGroupIndex = 0;
    std::map<u_long, grp_ptr> _groupList;
// методы работы с группами
public:
    grp_ptr addGroup(CHAR_DATA* leader);
    void removeGroup(u_long uid);
    grp_ptr findGroup(int personUID);
    grp_ptr findGroup(char* leaderName);
    void runTests(CHAR_DATA* leader);
private:
    std::list<rq_ptr> _requestList;
    std::tuple<INV_R, CHAR_DATA *> tryMakeInvite(Group* grp, char* member);
    std::tuple<RQ_R, grp_ptr> tryAddRequest(CHAR_DATA* author, char* targetGroup);
public:
    // методы работы с заявками
    void printRequestList(CHAR_DATA* ch);
    void makeInvite(CHAR_DATA* leader, char* targetPerson);
    void makeRequest(CHAR_DATA* author, char* targetGroup);
    void revokeRequest(CHAR_DATA* author, char* targetGroup);
    void acceptInvite(CHAR_DATA* who, char* author);
    void deleteRequest(Request * r);
    Request* findRequest(const char* targetPerson, const char* group, RQ_TYPE type);
};

#endif //BYLINS_GRP_MAIN_H