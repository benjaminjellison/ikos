/*******************************************************************************
 *
 * \file
 * \brief Forward fixpoint iterators of varying complexity and precision.
 *
 * The interleaved fixpoint iterator is described in G. Amato and F. Scozzari's
 * paper: Localizing widening and narrowing. In Proceedings of SAS 2013,
 * pages 25-42. LNCS 7935, 2013.
 *
 * Author: Arnaud J. Venet
 *
 * Contact: ikos@lists.nasa.gov
 *
 * Notices:
 *
 * Copyright (c) 2011-2019 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Disclaimers:
 *
 * No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF
 * ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS,
 * ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE
 * ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO
 * THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN
 * ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS,
 * RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
 * RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY
 * DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE,
 * IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
 *
 * Waiver and Indemnity:  RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST
 * THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL
 * AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS
 * IN ANY LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH
 * USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
 * RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD
 * HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS,
 * AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.
 * RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE,
 * UNILATERAL TERMINATION OF THIS AGREEMENT.
 *
 ******************************************************************************/

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>

#include <ikos/core/fixpoint/fixpoint_iterator.hpp>
#include <ikos/core/fixpoint/wto.hpp>

namespace ikos {
namespace core {

namespace interleaved_fwd_fixpoint_iterator_impl {

template < typename GraphRef, typename AbstractValue, typename GraphTrait >
class WtoIterator;

template < typename GraphRef, typename AbstractValue, typename GraphTrait >
class WtoProcessor;

} // end namespace interleaved_fwd_fixpoint_iterator_impl

template < typename GraphRef,
           typename AbstractValue,
           typename GraphTrait = GraphTraits< GraphRef > >
class InterleavedFwdFixpointIterator
    : public ForwardFixpointIterator< GraphRef, AbstractValue, GraphTrait > {
  friend class interleaved_fwd_fixpoint_iterator_impl::
      WtoIterator< GraphRef, AbstractValue, GraphTrait >;

private:
  using NodeRef = typename GraphTrait::NodeRef;
  using InvariantTable = std::unordered_map< NodeRef, AbstractValue >;
  using InvariantTablePtr = std::shared_ptr< InvariantTable >;
  using WtoT = Wto< GraphRef, GraphTrait >;
  using WtoIterator = interleaved_fwd_fixpoint_iterator_impl::
      WtoIterator< GraphRef, AbstractValue, GraphTrait >;
  using WtoProcessor = interleaved_fwd_fixpoint_iterator_impl::
      WtoProcessor< GraphRef, AbstractValue, GraphTrait >;

private:
  GraphRef _cfg;
  WtoT _wto;
  InvariantTablePtr _pre, _post;

public:
  /// \brief Create an interleaved forward fixpoint iterator
  explicit InterleavedFwdFixpointIterator(GraphRef cfg)
      : _cfg(cfg),
        _wto(cfg),
        _pre(std::make_shared< InvariantTable >()),
        _post(std::make_shared< InvariantTable >()) {}

  /// \brief Copy constructor
  InterleavedFwdFixpointIterator(const InterleavedFwdFixpointIterator&) =
      default;

  /// \brief Move constructor
  InterleavedFwdFixpointIterator(InterleavedFwdFixpointIterator&&) = default;

  /// \brief Copy assignment operator
  InterleavedFwdFixpointIterator& operator=(
      const InterleavedFwdFixpointIterator&) = default;

  /// \brief Move assignment operator
  InterleavedFwdFixpointIterator& operator=(InterleavedFwdFixpointIterator&&) =
      default;

  /// \brief Get the control flow graph
  GraphRef cfg() const { return this->_cfg; }

  /// \brief Get the weak topological order of the graph
  const WtoT& wto() const { return this->_wto; }

private:
  /// \brief Set the invariant for the given node
  static void set(const InvariantTablePtr& table,
                  NodeRef node,
                  AbstractValue inv) {
    auto it = table->find(node);
    if (it != table->end()) {
      it->second = std::move(inv);
    } else {
      table->emplace(node, std::move(inv));
    }
  }

  /// \brief Set the pre invariant for the given node
  void set_pre(NodeRef node, AbstractValue inv) {
    this->set(this->_pre, node, std::move(inv));
  }

  /// \brief Set the post invariant for the given node
  void set_post(NodeRef node, AbstractValue inv) {
    this->set(this->_post, node, std::move(inv));
  }

  /// \brief Get the invariant for the given node
  static const AbstractValue& get(const InvariantTablePtr& table,
                                  NodeRef node) {
    auto it = table->find(node);
    if (it != table->end()) {
      return it->second;
    } else {
      auto res = table->emplace(node, AbstractValue::bottom());
      return res.first->second;
    }
  }

public:
  /// \brief Get the pre invariant for the given node
  const AbstractValue& pre(NodeRef node) const {
    return this->get(this->_pre, node);
  }

  /// \brief Get the post invariant for the given node
  const AbstractValue& post(NodeRef node) const {
    return this->get(this->_post, node);
  }

  /// \brief Extrapolate the new state after an increasing iteration
  ///
  /// This is called after each iteration of a cycle, until the fixpoint is
  /// reached. In order to converge, the widening operator must be applied.
  /// This method gives the user the ability to use different widening
  /// strategies.
  ///
  /// By default, it applies a join for the first iteration, and then the
  /// widening until it reaches the fixpoint.
  ///
  /// \param head Head of the cycle
  /// \param iteration Iteration number
  /// \param before Abstract value before the iteration
  /// \param after Abstract value after the iteration
  virtual AbstractValue extrapolate(NodeRef head,
                                    unsigned iteration,
                                    AbstractValue before,
                                    AbstractValue after) {
    ikos_ignore(head);
    if (iteration <= 1) {
      before.join_iter_with(after);
    } else {
      before.widen_with(after);
    }
    return before;
  }

  /// \brief Check if the increasing iterations fixpoint is reached
  ///
  /// \param before Abstract value before the iteration
  /// \param after Abstract value after the iteration
  virtual bool is_increasing_iterations_fixpoint(const AbstractValue& before,
                                                 const AbstractValue& after) {
    return after.leq(before);
  }

  /// \brief Refine the new state after a decreasing iteration
  ///
  /// This is called after each iteration of a cycle, until the post fixpoint
  /// is reached. In order to converge, the narrowing operator must be applied.
  /// This method gives the user the ability to use different narrowing
  /// strategies.
  ///
  /// By default, it applies the narrowing until it reaches the post fixpoint.
  ///
  /// \param head Head of the cycle
  /// \param iteration Iteration number
  /// \param before Abstract value before the iteration
  /// \param after Abstract value after the iteration
  virtual AbstractValue refine(NodeRef head,
                               unsigned iteration,
                               AbstractValue before,
                               AbstractValue after) {
    ikos_ignore(head);
    ikos_ignore(iteration);
    before.narrow_with(after);
    return before;
  }

  /// \brief Check if the decreasing iterations fixpoint is reached
  ///
  /// \param before Abstract value before the iteration
  /// \param after Abstract value after the iteration
  virtual bool is_decreasing_iterations_fixpoint(const AbstractValue& before,
                                                 const AbstractValue& after) {
    return before.leq(after);
  }

  /// \brief Compute the fixpoint with the given initial abstract value
  void run(AbstractValue init) {
    this->set_pre(GraphTrait::entry(this->_cfg), std::move(init));
    WtoIterator iterator(*this);
    this->_wto.accept(iterator);
    WtoProcessor processor(*this);
    this->_wto.accept(processor);
  }

  /// \brief Clear the current fixpoint
  void clear() {
    this->_pre = std::make_shared< InvariantTable >();
    this->_post = std::make_shared< InvariantTable >();
  }

  /// \brief Destructor
  ~InterleavedFwdFixpointIterator() override = default;

}; // end class InterleavedFwdFixpointIterator

namespace interleaved_fwd_fixpoint_iterator_impl {

template < typename GraphRef, typename AbstractValue, typename GraphTrait >
class WtoIterator : public WtoComponentVisitor< GraphRef, GraphTrait > {
public:
  using InterleavedIterator =
      InterleavedFwdFixpointIterator< GraphRef, AbstractValue, GraphTrait >;
  using NodeRef = typename GraphTrait::NodeRef;
  using WtoVertexT = WtoVertex< GraphRef, GraphTrait >;
  using WtoCycleT = WtoCycle< GraphRef, GraphTrait >;
  using WtoT = Wto< GraphRef, GraphTrait >;
  using WtoNestingT = typename WtoT::WtoNestingT;

private:
  enum IterationKind { Increasing, Decreasing };

private:
  InterleavedIterator& _iterator;

public:
  explicit WtoIterator(InterleavedIterator& iterator) : _iterator(iterator) {}

  void visit(const WtoVertexT& vertex) override {
    NodeRef node = vertex.node();
    AbstractValue pre = AbstractValue::bottom();

    // Use the invariant for the entry point
    if (node == GraphTrait::entry(this->_iterator.cfg())) {
      pre = this->_iterator.pre(node);
    }

    // Collect invariants from incoming edges
    for (auto it = GraphTrait::predecessor_begin(node),
              et = GraphTrait::predecessor_end(node);
         it != et;
         ++it) {
      NodeRef pred = *it;
      pre.join_with(
          this->_iterator.analyze_edge(pred, node, this->_iterator.post(pred)));
    }

    this->_iterator.set_pre(node, pre);
    this->_iterator.set_post(node, this->_iterator.analyze_node(node, pre));
  }

  void visit(const WtoCycleT& cycle) override {
    NodeRef head = cycle.head();
    WtoNestingT cycle_nesting = this->_iterator.wto().nesting(head);
    AbstractValue pre = AbstractValue::bottom();

    // Collect invariants from incoming edges
    for (auto it = GraphTrait::predecessor_begin(head),
              et = GraphTrait::predecessor_end(head);
         it != et;
         ++it) {
      NodeRef pred = *it;
      if (!(this->_iterator.wto().nesting(pred) > cycle_nesting)) {
        pre.join_with(this->_iterator.analyze_edge(pred,
                                                   head,
                                                   this->_iterator.post(pred)));
      }
    }

    // Fixpoint iterations
    IterationKind kind = Increasing;
    for (unsigned iteration = 1;; ++iteration) {
      this->_iterator.set_pre(head, pre);
      this->_iterator.set_post(head, this->_iterator.analyze_node(head, pre));

      for (auto it = cycle.begin(), et = cycle.end(); it != et; ++it) {
        it->accept(*this);
      }

      // Invariant from the head of the loop
      AbstractValue new_pre_in = AbstractValue::bottom();

      for (auto it = GraphTrait::predecessor_begin(head),
                et = GraphTrait::predecessor_end(head);
           it != et;
           ++it) {
        NodeRef pred = *it;
        if (!(this->_iterator.wto().nesting(pred) > cycle_nesting)) {
          new_pre_in.join_with(
              this->_iterator.analyze_edge(pred,
                                           head,
                                           this->_iterator.post(pred)));
        }
      }

      // Invariant from the tail of the loop
      AbstractValue new_pre_back = AbstractValue::bottom();

      for (auto it = GraphTrait::predecessor_begin(head),
                et = GraphTrait::predecessor_end(head);
           it != et;
           ++it) {
        NodeRef pred = *it;
        if (this->_iterator.wto().nesting(pred) > cycle_nesting) {
          new_pre_back.join_with(
              this->_iterator.analyze_edge(pred,
                                           head,
                                           this->_iterator.post(pred)));
        }
      }

      AbstractValue new_pre(std::move(new_pre_in));
      new_pre.join_loop_with(new_pre_back);

      if (kind == Increasing) {
        // Increasing iteration with widening
        if (this->_iterator.is_increasing_iterations_fixpoint(pre, new_pre)) {
          // Post-fixpoint reached
          // Use this iteration as a decreasing iteration
          kind = Decreasing;
          iteration = 1;
        } else {
          pre = this->_iterator.extrapolate(head,
                                            iteration,
                                            std::move(pre),
                                            std::move(new_pre));
        }
      }

      if (kind == Decreasing) {
        // Decreasing iteration with narrowing
        new_pre =
            this->_iterator.refine(head, iteration, pre, std::move(new_pre));
        if (this->_iterator.is_decreasing_iterations_fixpoint(pre, new_pre)) {
          // No more refinement possible
          this->_iterator.set_pre(head, std::move(new_pre));
          break;
        } else {
          pre = std::move(new_pre);
        }
      }
    }
  }

}; // end class WtoIterator

template < typename GraphRef, typename AbstractValue, typename GraphTrait >
class WtoProcessor : public WtoComponentVisitor< GraphRef, GraphTrait > {
public:
  using InterleavedIterator =
      InterleavedFwdFixpointIterator< GraphRef, AbstractValue, GraphTrait >;
  using NodeRef = typename GraphTrait::NodeRef;
  using WtoVertexT = WtoVertex< GraphRef, GraphTrait >;
  using WtoCycleT = WtoCycle< GraphRef, GraphTrait >;

private:
  InterleavedIterator& _iterator;

public:
  explicit WtoProcessor(InterleavedIterator& iterator) : _iterator(iterator) {}

  void visit(const WtoVertexT& vertex) override {
    NodeRef node = vertex.node();
    this->_iterator.process_pre(node, this->_iterator.pre(node));
    this->_iterator.process_post(node, this->_iterator.post(node));
  }

  void visit(const WtoCycleT& cycle) override {
    NodeRef head = cycle.head();
    this->_iterator.process_pre(head, this->_iterator.pre(head));
    this->_iterator.process_post(head, this->_iterator.post(head));

    for (auto it = cycle.begin(), et = cycle.end(); it != et; ++it) {
      it->accept(*this);
    }
  }

}; // end class WtoProcessor

} // end namespace interleaved_fwd_fixpoint_iterator_impl

} // end namespace core
} // end namespace ikos
