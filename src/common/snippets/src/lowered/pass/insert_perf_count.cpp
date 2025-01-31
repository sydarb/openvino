// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "snippets/lowered/pass/insert_perf_count.hpp"
#include "snippets/lowered/linear_ir.hpp"
#include "snippets/snippets_isa.hpp"
#include "snippets/itt.hpp"

namespace ov {
namespace snippets {
namespace lowered {
namespace pass {

bool InsertPerfCount::run(LinearIR& linear_ir) {
    OV_ITT_SCOPED_TASK(ov::pass::itt::domains::SnippetsTransform, "Snippets::InsertPerfCount")
    if (linear_ir.empty())
        return false;

    auto is_parameter = [](const std::shared_ptr<ov::Node>& node) {
        return ov::is_type<ov::op::v0::Parameter>(node);
    };
    auto is_result = [](const std::shared_ptr<ov::Node>& node) {
        return ov::is_type<ov::op::v0::Result>(node);
    };

    // mark perf_count_begin and perf_count_end position
    auto perf_count_begin_pos = linear_ir.cbegin();
    auto perf_count_end_pos = perf_count_begin_pos;
    bool first_result_marked = false;
    for (auto expr_it = linear_ir.cbegin(); expr_it != linear_ir.cend(); expr_it++) {
        const auto expr = *expr_it;
        const auto& node = expr->get_node();
        if (is_parameter(node))
            perf_count_begin_pos = expr_it;

        if (is_result(node) && !first_result_marked) {
            perf_count_end_pos = expr_it;
            first_result_marked = true;
        }
    }

    // insert perf_count_begin after last parameter
    // linear_ir.insert has insert before behavior, need move to next.
    perf_count_begin_pos = std::next(perf_count_begin_pos);
    const auto& perf_count_begin = std::make_shared<op::PerfCountBegin>();
    const auto& perf_count_begin_expr = linear_ir.create_expression(perf_count_begin, std::vector<PortConnectorPtr>{});
    linear_ir.insert(perf_count_begin_pos, perf_count_begin_expr);

    // insert perf_count_end before first result
    const auto& perf_count_end = std::make_shared<op::PerfCountEnd>(perf_count_begin->output(0));
    perf_count_end->set_friendly_name("last_parameter_to_first_result");
    const auto& perf_count_end_expr = linear_ir.create_expression(perf_count_end, std::vector<PortConnectorPtr>{});
    linear_ir.insert(perf_count_end_pos, perf_count_end_expr);

    return true;
}

} // namespace pass
} // namespace lowered
} // namespace snippets
} // namespace ov
