/**
 * @file hsm_fw.c
 * @brief 閫氱敤 HSM 妗嗘灦瀹炵幇
 */

#include "hsm_fw.h"

#if HsmFwUse

#include "esp_log.h"
#include <string.h>

void hsm_init(hsm_t *me, const hsm_node_t *init_state, void *user)
{
    if (me == NULL || init_state == NULL) return;
    memset(me, 0, sizeof(*me));
    me->current = init_state;
    me->history = init_state;
    me->user = user;
    if (init_state->on_entry) {
        init_state->on_entry(me, NULL, 0);
    }
}

void hsm_transition(hsm_t *me, const hsm_node_t *target)
{
    if (me == NULL || target == NULL) return;
    const hsm_node_t *old = me->current;
    if (old == target) return;

    if (old && old->on_exit) old->on_exit(me);
    me->current = target;
    /* 璁板綍鐖跺眰绾у巻鍙?*/
    if (target->parent && target->parent->has_history) {
        /* 鍘嗗彶鐢卞浣嶉€昏緫浣跨敤 */
    }
    if (target->on_entry) target->on_entry(me, NULL, 0);
    ESP_LOGI("hsm", "%s -> %s", old ? old->name : "?", target->name);
}

void hsm_dispatch(hsm_t *me, uint16_t event_id, void *payload, uint8_t len)
{
    if (me == NULL || me->current == NULL) return;

    const hsm_node_t *s = me->current;
    /* 娌跨姸鎬佹爲鍚戜笂鏌ユ壘锛氭湰灞備紭鍏堬紝閫愮骇鍚?parent 濮旀墭 */
    while (s != NULL) {
        for (uint8_t i = 0; i < s->trans_cnt; i++) {
            if (s->trans[i].event_id == event_id) {
                const hsm_node_t *target = s->trans[i].target;
                if (target != NULL) {
                    /* 鎵ц閫€鍑洪摼锛氬綋鍓嶇姸鎬佸強鍏剁鍏堢洿鍒扮洰鏍囧叡鍚岀鍏?*/
                    const hsm_node_t *exit_s = me->current;
                    while (exit_s && exit_s != target) {
                        if (exit_s->on_exit) exit_s->on_exit(me);
                        if (exit_s->parent) {
                            if (exit_s->parent == target) break;
                        }
                        exit_s = exit_s->parent;
                    }
                    /* 鐩爣鑻ユ棤 on_entry 鍙傛暟锛屽垯璺宠繃锛堥伩鍏嶅弻璋冪敤锛?*/
                    bool skip_entry = false;
                    if (target == me->current) skip_entry = true;   /* 鍘熷湴浜嬩欢 */
                    me->current = target;
                    if (!skip_entry && target->on_entry) {
                        target->on_entry(me, payload, len);
                    }
                    if (target->parent && target->parent->has_history) {
                        me->history = target;   /* 璁板綍鍘嗗彶瀛愭€?*/
                    }
                }
                return;   /* 浜嬩欢宸叉秷璐?*/
            }
        }
        s = s->parent;   /* 鍚戠埗绾у鎵?*/
    }
    /* 鏈鐞嗕簨浠讹細蹇界暐锛堝彲鎵╁睍涓哄叏灞€鏈鐞嗗洖璋冿級 */
}

const char *hsm_state_name(const hsm_t *me)
{
    if (me == NULL || me->current == NULL) return "none";
    return me->current->name;
}

void hsm_reset_to_history(hsm_t *me)
{
    if (me == NULL || me->current == NULL) return;
    const hsm_node_t *old = me->current;
    if (old->on_exit) old->on_exit(me);
    me->current = (me->history != NULL) ? me->history : me->current;
    if (me->current->on_entry) me->current->on_entry(me, NULL, 0);
}

/* init 琛ㄦ帴鍏ワ細妗嗘灦鏃犲叏灞€鍒濆鍖栧姩浣滐紝鐧昏鍗犱綅 */
void hsm_fw_init_stub(void *arg)
{
    (void)arg;
    ESP_LOGI("hsm_fw", "HSM framework ready");
}

#endif /* HsmFwUse */