#pragma once

// FIX 4.4 保证金更新消息 (BI) 字段定义
// 基于产品文档：Margin Update Message (tag 35=BI)
namespace fixcustom {

// 消息类型：保证金更新
inline constexpr const char kMsgTypeMarginUpdate[] = "BI";

// 标准字段
inline constexpr int TAG_ACCOUNT = 1;        // 账户ID (Account)
inline constexpr int TAG_CURRENCY = 15;      // 货币类型 (Currency)

// 自定义字段编号
inline constexpr int TAG_MARGIN_VALUE = 20002;   // 保证金总额 (MarginValue)
inline constexpr int TAG_MARGIN_LEVEL = 20003;   // 已使用保证金比例% (MarginLevel)
inline constexpr int TAG_MARGIN_EXCESS = 899;    // 超额保证金 (MarginExcess)

// 货币枚举值
inline constexpr int CURRENCY_USD = 2;       // 美元

}


