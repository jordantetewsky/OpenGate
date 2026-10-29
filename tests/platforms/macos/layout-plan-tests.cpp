// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <cstdio>
#include "grouplayout.h"
#include "windowsnap.h"
#include "viewport.h"
#include "droppolicy.h"
#include "dockfeedback.h"
#include "nativearrangementplan.h"
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    int failures = 0;
    const auto check = [&](bool pass, const char *message) { if (!pass) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); } };
    const QRect frame(100, 100, 800, 600);
    check(Unbound::dropCaption(Unbound::DropPlacement::Tab, "Media Pool", false) == "Add as tab to Media Pool",
        "tab feedback names the destination and action");
    check(Unbound::dropCaption(Unbound::DropPlacement::Below, "Source Monitor", false) == "Dock below Source Monitor" &&
        Unbound::dropCaption(Unbound::DropPlacement::Right, "", true) == "Dock right of entire group",
        "split feedback distinguishes leaf and whole-group scope");
    using Placement = Unbound::DropPlacement;
    for (const QSize size : {QSize(80,60), QSize(800,600), QSize(3000,1600)}) {
        const QRect surface(QPoint(-300,90), size);
        for (int x = surface.left()-1; x <= surface.right()+1; ++x)
            for (int y = surface.top()-1; y <= surface.bottom()+1; y += 7) {
                const QPoint point(x,y);
                const auto selection = Unbound::selectPanelDrop(surface, point);
                check(selection.placement==Placement::None || surface.contains(point),
                    "explicit targets never extend onto the desktop");
                if (selection.placement != Placement::None)
                    check(surface.contains(selection.preview), "preview stays inside its destination");
            }
        check(Unbound::selectPanelDrop(surface, surface.center()).placement == Placement::Tab,
            "tab region remains reachable at all tested window sizes");
        const QPoint left(surface.left()+1, surface.center().y());
        check(Unbound::selectPanelDrop(surface, left).placement == Placement::Left,
            "an edge is reachable without hunting for a centered icon");
        check(Unbound::selectPanelDrop(surface, QPoint(surface.left()-1,left.y()), Placement::Left).placement == Placement::None,
            "leaving an active target clears docking even with retention enabled");
    }
    const QPoint boundary(frame.left()+40, frame.center().y());
    check(Unbound::selectPanelDrop(frame, boundary, Placement::Left).placement == Placement::Left &&
        Unbound::selectPanelDrop(frame, boundary+QPoint(5,0), Placement::Left).placement == Placement::None,
        "boundary jitter is retained briefly without trapping the pointer in a split");
    check(Unbound::selectPanelDrop(frame,QPoint(frame.left()+100,frame.top()+100)).placement==Placement::None,
        "ordinary window interior is a neutral free-floating region");

    int offered = 0;
    for (int x = -100; x < 1200; x += 10) for (int y = -100; y < 1000; y += 10)
        offered += Unbound::localDockZone(frame, QPoint(x, y)).target.isValid();
    check(offered < 100, "targets occupy isolated approach areas, not the whole screen or perimeter");
    for (int i = 0; i < 4; ++i) {
        QPoint edge = i == 0 ? QPoint(frame.left(), frame.center().y()) : i == 1 ? QPoint(frame.right(), frame.center().y()) :
            i == 2 ? QPoint(frame.center().x(), frame.top()) : QPoint(frame.center().x(), frame.bottom());
        auto zone = Unbound::localDockZone(frame, edge);
        check(zone.active && zone.edge != Qt::Edge(0), "edge anchor predicts a split");
        check(Unbound::localDockZone(frame, edge + QPoint(2, 2)).target == zone.target, "target is stable through small pointer movement");
    }
    auto tab = Unbound::localDockZone(frame, frame.center());
    check(tab.active && tab.edge == Qt::Edge(0) && tab.preview == frame, "tab preview is distinct from a half split");
    auto compass = Unbound::dockCompassZones(frame);
    check(compass.size() == 5, "hover compass offers four split directions and one tab");
    for (int i = 0; i < compass.size(); ++i) {
        check(frame.contains(compass[i].target) && compass[i].target.size() == QSize(24, 24), "compass targets stay compact inside their destination");
        for (int j = i + 1; j < compass.size(); ++j) check(!compass[i].target.intersects(compass[j].target), "tab and split hit targets never overlap");
    }
    auto tree = Unbound::splitGroupTree({{"panel", "Inspector"}}, {{"panel", "MediaPool"}}, Qt::Orientation(0));
    tree = Unbound::insertGroupTree(tree, {{"panel", "Effects"}}, "Inspector", Qt::Orientation(0), false);
    QStringList names;
    check(Unbound::groupTreeNames(tree, names) && names.size() == 3 && tree["children"].toArray().size() == 3, "adding another tab extends the existing tab stack");
    auto split = Unbound::splitGroupTree(tree, {{"panel", "Timeline"}}, Qt::Vertical);
    names.clear(); check(Unbound::groupTreeNames(split, names) && names.size() == 4, "tab stacks coexist with split groups");
    auto removed = Unbound::removeGroupLeaf(split, "Timeline");
    check(removed["orientation"] == "tabs", "removing a split neighbor preserves its tab stack");
    {
        const auto inserted=Unbound::insertGroupTree({{"panel","media"}},{{"panel","inspector"}},"media",Qt::Orientation(0),false);
        check(inserted["selected"]==1,"dropping a panel as a new tab selects the newly inserted panel");
        QJsonObject tabs{{"orientation","tabs"},{"selected",2},{"children",QJsonArray{
            QJsonObject{{"panel","a"}},QJsonObject{{"panel","b"}},QJsonObject{{"panel","c"}},QJsonObject{{"panel","d"}}}}};
        const auto earlier=Unbound::removeGroupLeaf(tabs,"a");
        check(earlier["selected"]==1 && earlier["children"].toArray()[1].toObject()["panel"]=="c",
            "detaching an earlier tab preserves the selected panel after indexes shift");
        const auto selected=Unbound::removeGroupLeaf(tabs,"c");
        check(selected["selected"]==2 && selected["children"].toArray()[2].toObject()["panel"]=="d",
            "detaching the selected tab selects its surviving next neighbor");
        tabs["selected"]=3; const auto last=Unbound::removeGroupLeaf(tabs,"d");
        check(last["selected"]==2,"detaching the last selected tab leaves a valid previous selection");
    }
    auto top = Unbound::splitGroupTree({{"panel", "Inspector"}}, {{"panel", "MediaPool"}}, Qt::Horizontal);
    auto puzzle = Unbound::splitGroupTree(top, {{"panel", "Source"}}, Qt::Vertical);
    names.clear(); check(Unbound::groupTreeNames(puzzle, names) && names.size() == 3 &&
        puzzle["children"].toArray()[0].toObject()["orientation"] == "horizontal",
        "Inspector and Media Pool can sit side by side with Source spanning below");
    const QPoint outer(frame.right() + 24, frame.center().y());
    check(Unbound::outerDockZone(frame, outer).active && !Unbound::localDockZone(frame, outer).active,
        "whole-group target is separate from panel edge targets");
    // Merge independent arrangements repeatedly; preserve each subtree and every panel.
    QJsonObject arrangement;
    QStringList expected;
    for (int group = 0; group < 6; ++group) {
        QJsonObject subtree;
        for (int panel = 0; panel < 4; ++panel) {
            const QString name = QString("group%1-panel%2").arg(group).arg(panel);
            expected.append(name);
            const QJsonObject leaf{{"panel", name}};
            subtree = subtree.isEmpty() ? leaf : Unbound::splitGroupTree(subtree, leaf,
                panel % 2 ? Qt::Horizontal : Qt::Vertical);
        }
        const auto prior = arrangement;
        arrangement = arrangement.isEmpty() ? subtree : Unbound::insertGroupTree(arrangement, subtree,
            QString(), group % 2 ? Qt::Vertical : Qt::Horizontal, false);
        names.clear();
        check(Unbound::groupTreeNames(arrangement, names) && names == expected,
            "N groups merge without losing, duplicating or reordering panels");
        if (!prior.isEmpty()) check(arrangement["children"].toArray()[0].toObject() == prior &&
            arrangement["children"].toArray()[1].toObject() == subtree,
            "whole-group merge preserves internal split geometry and structure");
    }
    // A nested destination must not split unrelated neighbors.
    const auto original = arrangement;
    const QJsonObject extra{{"panel", "extra"}};
    arrangement = Unbound::insertGroupTree(arrangement, extra, "group2-panel1", Qt::Horizontal, true);
    names.clear();
    check(Unbound::groupTreeNames(arrangement, names) && names.size() == 25,
        "a panel can be inserted into an interior leaf of a large arrangement");
    arrangement = Unbound::removeGroupLeaf(arrangement, "extra");
    check(arrangement == original, "tearing the inserted panel out restores the original tree exactly");
    for (const auto &name : expected) {
        arrangement = Unbound::removeGroupLeaf(arrangement, name);
        names.clear();
        check(arrangement.isEmpty() || (Unbound::groupTreeNames(arrangement, names) && !names.contains(name)),
            "successive tear-offs leave a valid arrangement without orphan split nodes");
    }
    check(arrangement.isEmpty(), "removing the final panel leaves no empty arrangement");
    const QRect desktop(0, 0, 1920, 1080), partial(-450, 50, 600, 500);
    check(Unbound::keepGripReachable(partial, {desktop}) == partial, "intentional partial off-screen placement stays unchanged");
    const auto recovered = Unbound::keepGripReachable(QRect(4000, -3000, 2400, 1500), {desktop});
    check(recovered.size() == QSize(2400, 1500) && recovered.top() >= 0 && recovered.left() <= desktop.right() - 95,
        "lost windows retain size while a draggable strip is rescued");
    {
        Unbound::NativeArrangementPlan saved; saved.page="edit";
        auto tabs=Unbound::splitGroupTree({{"panel","source"}},{{"panel","inspector"}},Qt::Orientation(0)); tabs["selected"]=1;
        saved.tree=Unbound::splitGroupTree({{"panel",Unbound::nativeRemainderId()}},tabs,Qt::Horizontal); saved.hidden.insert("source");
        Unbound::NativeArrangementPlan restored;
        check(Unbound::NativeArrangementPlan::parse(saved.json(),"edit",{"source","inspector"},restored).isEmpty() && restored.json()==saved.json(),
            "native saved plans preserve nested tabs, split ratios, page identity and hidden panels exactly");
        const auto baseline=restored.json();
        auto reject=[&](QJsonObject bad,const QString &page=QStringLiteral("edit")) {
            check(!Unbound::NativeArrangementPlan::parse(bad,page,{"source","inspector"},restored).isEmpty() && restored.json()==baseline,
                "invalid native plans reject atomically without changing the last valid plan");
        };
        reject(saved.json(),"color"); auto bad=saved.json(); bad["version"]=99; reject(bad);
        bad=saved.json(); bad["panels"]=QJsonObject{{"source",QJsonObject{{"hidden","yes"}}}}; reject(bad);
        bad=saved.json(); auto tree=saved.tree; tree["sizes"]=QJsonArray{-1,2}; bad["tree"]=tree; reject(bad);
        tree=saved.tree; tree["sizes"]=QJsonArray{1.5,2}; bad["tree"]=tree; reject(bad);
        tree=saved.tree; tree["sizes"]=QJsonArray{1}; bad["tree"]=tree; reject(bad);
        tree=saved.tree; auto children=tree["children"].toArray(); auto invalidTabs=tabs; invalidTabs["selected"]=5;
        children[1]=invalidTabs; tree["children"]=children; bad["tree"]=tree; reject(bad);
        bad=saved.json(); bad["tree"]=tabs; reject(bad);
        bad=saved.json(); bad["tree"]=Unbound::splitGroupTree({{"panel",Unbound::nativeRemainderId()}},{{"panel","unavailable"}},Qt::Horizontal); reject(bad);
        Unbound::NativeArrangementPlan scoped; scoped.page="edit"; scoped.anchor="source"; scoped.tree=tabs; scoped.hidden.insert("inspector");
        check(Unbound::NativeArrangementPlan::parse(scoped.json(),"edit",{"source","inspector"},restored).isEmpty() && restored.json()==scoped.json(),
            "scoped plans preserve a specific native anchor, tab selection and closed panel states");
        scoped.tree={{"panel","inspector"}};
        check(Unbound::NativeArrangementPlan::parse(scoped.json(),"edit",{"source","inspector"},restored).isEmpty(),
            "a scoped plan retains anchor identity after the anchor panel floats away");
        auto scopedBad=scoped.json(); scopedBad["anchor"]="missing";
        check(!Unbound::NativeArrangementPlan::parse(scopedBad,"edit",{"source","inspector"},restored).isEmpty(),
            "a scoped plan cannot claim an unknown native anchor");
        scopedBad=scoped.json(); scopedBad["anchor"]=12;
        check(!Unbound::NativeArrangementPlan::parse(scopedBad,"edit",{"source","inspector"},restored).isEmpty(),
            "malformed anchor metadata cannot masquerade as a whole-page plan");
        scopedBad=saved.json(); scopedBad["anchor"]="source";
        check(!Unbound::NativeArrangementPlan::parse(scopedBad,"edit",{"source","inspector"},restored).isEmpty(),
            "a scoped native plan cannot include the entire native page remainder");
    }
    if (!failures) std::puts("PASS: regional drop policy, recursive 24-panel merges/tear-offs, legacy geometry and viewport recovery (no GUI windows)");
    return failures ? 1 : 0;
}
