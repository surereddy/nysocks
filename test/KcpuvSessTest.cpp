#include "KcpuvTest.h"
#include "kcpuv_sess.h"
#include <iostream>
#include <string.h>

namespace kcpuv_test {

using namespace std;

static testing::MockFunction<void(const char *)> *test_callback1;

class KcpuvSessTest : public testing::Test {
protected:
  KcpuvSessTest(){};
  virtual ~KcpuvSessTest(){};
};

TEST_F(KcpuvSessTest, push_the_sess_to_sess_list_when_created) {
  kcpuv_initialize();

  kcpuv_sess *sess = kcpuv_create();
  kcpuv_sess_list *sess_list = kcpuv_get_sess_list();

  EXPECT_EQ(sess_list->len, 1);
  EXPECT_TRUE(sess_list->list->next);

  kcpuv_free(sess);

  EXPECT_EQ(sess_list->len, 0);
  EXPECT_FALSE(sess_list->list->next);

  kcpuv_destruct();
}

void recver_cb(kcpuv_sess *sess, char *data, int len) {
  test_callback1->Call(data);
  delete test_callback1;
  kcpuv_destroy_loop();
}

// TODO: valgrid possiblely lost
TEST_F(KcpuvSessTest, transfer_one_packet) {
  kcpuv_initialize();

  test_callback1 = new testing::MockFunction<void(const char *)>();
  kcpuv_sess *sender = kcpuv_create();
  kcpuv_sess *recver = kcpuv_create();
  int send_port = 12305;
  int receive_port = 12306;
  char *msg = "Hello";

  // bind local
  kcpuv_listen(recver, receive_port, &recver_cb);
  kcpuv_init_send(sender, "127.0.0.1", receive_port);

  kcpuv_send(sender, msg, strlen(msg));

  EXPECT_CALL(*test_callback1, Call(StrEq("Hello"))).Times(1);

  kcpuv_start_loop();

  kcpuv_free(sender);
  kcpuv_free(recver);
  kcpuv_destruct();
}

static testing::MockFunction<void(int)> *test_callback2;

void recver_cb2(kcpuv_sess *sess, char *data, int len) {
  test_callback2->Call(len);
  delete test_callback2;
  kcpuv_destroy_loop();
}

TEST_F(KcpuvSessTest, transfer_multiple_packets) {
  kcpuv_initialize();

  test_callback2 = new testing::MockFunction<void(int)>();

  kcpuv_sess *sender = kcpuv_create();
  kcpuv_sess *recver = kcpuv_create();
  int send_port = 12002;
  int receive_port = 12003;
  int size = 4096;
  char *msg = new char[size];

  memset(msg, 65, size);

  // bind local
  kcpuv_listen(recver, receive_port, &recver_cb2);
  kcpuv_init_send(sender, "127.0.0.1", receive_port);

  kcpuv_send(sender, msg, size);

  EXPECT_CALL(*test_callback2, Call(size)).Times(1);

  kcpuv_start_loop();
  kcpuv_destruct();

  delete msg;
}

static testing::MockFunction<void(int)> *test_callback22;

void recver_cb22(kcpuv_sess *sess, char *data, int len) {
  test_callback22->Call(len);
  delete test_callback22;
  kcpuv_destroy_loop();
}

TEST_F(KcpuvSessTest, mock_implementation) {
  kcpuv_initialize();

  test_callback22 = new testing::MockFunction<void(int)>();

  char *addr = "127.0.0.1";
  int sender_port = 8888;
  int receiver_port = 8889;
  char *msg = "hello";
  int msg_len = 5;

  kcpuv_sess *sender = kcpuv_create();
  kcpuv_sess *receiver = kcpuv_create();

  kcpuv_listen(receiver, receiver_port, &recver_cb22);
  kcpuv_init_send(sender, addr, receiver_port);

  kcpuv_send(sender, msg, msg_len);

  EXPECT_CALL(*test_callback22, Call(_)).Times(1);

  kcpuv_start_loop();

  kcpuv_destruct();
}

static testing::MockFunction<void(void)> *test_callback3;

static void close_cb(kcpuv_sess *sess, const char *error_msg) {
  test_callback3->Call();
  delete test_callback3;
}

TEST_F(KcpuvSessTest, on_close_cb) {
  kcpuv_initialize();

  test_callback3 = new testing::MockFunction<void(void)>();

  kcpuv_sess *sender = kcpuv_create();

  kcpuv_bind_close(sender, &close_cb);

  EXPECT_CALL(*test_callback3, Call()).Times(1);

  kcpuv_close(sender, 0, NULL);

  kcpuv_destruct();
}

static testing::MockFunction<void(void)> *test_callback4;

static void close_cb2(kcpuv_sess *sess, const char *error_msg) {
  test_callback4->Call();
  delete test_callback4;
  kcpuv_destroy_loop();
}

TEST_F(KcpuvSessTest, one_close_should_close_the_other_side) {
  kcpuv_initialize();

  test_callback4 = new testing::MockFunction<void(void)>();

  kcpuv_sess *sender = kcpuv_create();
  kcpuv_sess *recver = kcpuv_create();
  int send_port = 12004;
  int receive_port = 12005;

  // bind local
  kcpuv_listen(recver, receive_port, NULL);
  kcpuv_init_send(sender, "127.0.0.1", receive_port);

  kcpuv_bind_close(recver, close_cb2);

  EXPECT_CALL(*test_callback4, Call()).Times(1);

  kcpuv_close(sender, 1, NULL);

  kcpuv_start_loop();

  kcpuv_destruct();
}

static testing::MockFunction<void(void)> *test_callback5;

static void close_cb3(kcpuv_sess *sess, const char *error_msg) {
  test_callback5->Call();
  delete test_callback5;
  kcpuv_destroy_loop();
}

TEST_F(KcpuvSessTest, timeout) {
  kcpuv_initialize();

  test_callback5 = new testing::MockFunction<void(void)>();

  kcpuv_sess *sender = kcpuv_create();
  int send_port = 12007;
  int receive_port = 12306;

  kcpuv_listen(sender, send_port, NULL);
  kcpuv_init_send(sender, "127.0.0.1", receive_port);

  sender->timeout = 100;
  kcpuv_bind_close(sender, &close_cb3);

  EXPECT_CALL(*test_callback5, Call()).Times(1);

  kcpuv_start_loop();

  kcpuv_stop_listen(sender);

  kcpuv_destruct();
}

TEST_F(KcpuvSessTest, kcpuv_get_address) {
  kcpuv_initialize();

  int bind_port = 8990;
  kcpuv_sess *sess = kcpuv_create();
  kcpuv_listen(sess, bind_port, NULL);

  char *ip_addr = new char[16];
  int port = 0;
  int namelen = 0;
  int rval = kcpuv_get_address(sess, ip_addr, &namelen, &port);

  EXPECT_EQ(rval, 0);
  EXPECT_EQ(port, bind_port);

  delete[] ip_addr;
  kcpuv_stop_listen(sess);
  kcpuv_destruct();
}

static testing::MockFunction<void(void)> *last_packet_addr_callback;

static void last_packet_addr_cb(kcpuv_sess *sess, char *error_msg, int len) {
  char *addr = new char[17];
  int port = 0;

  int namelen = kcpuv_get_last_packet_addr(sess, addr, &port);

  last_packet_addr_callback->Call();
  delete last_packet_addr_callback;

  delete[] addr;
  kcpuv_destroy_loop();
}

TEST_F(KcpuvSessTest, last_packet_addr) {
  kcpuv_initialize();

  last_packet_addr_callback = new testing::MockFunction<void(void)>();
  EXPECT_CALL(*last_packet_addr_callback, Call()).Times(1);

  kcpuv_sess *sess = kcpuv_create();
  kcpuv_sess *sender = kcpuv_create();

  kcpuv_set_save_last_packet_addr(sess, 1);
  kcpuv_listen(sess, 0, &last_packet_addr_cb);

  char *addr = new char[17];
  int namelen = 0;
  int port = 0;
  kcpuv_get_address(sess, addr, &namelen, &port);

  char *msg = "hello";
  kcpuv_init_send(sender, "127.0.0.1", port);
  kcpuv_send(sender, msg, strlen(msg));

  kcpuv_start_loop();

  delete[] addr;
  kcpuv_stop_listen(sess);
  kcpuv_close(sender, 0, NULL);
  kcpuv_close(sess, 0, NULL);
  kcpuv_destruct();
}
} // namespace kcpuv_test
